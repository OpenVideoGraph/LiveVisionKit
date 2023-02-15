//    *************************** LiveVisionKit ****************************
//    Copyright (C) 2022  Sebastian Di Marco (crowsinc.dev@gmail.com)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.
// 	  **********************************************************************

#include "DeblockingFilter.hpp"

#include "Utility/Drawing.hpp"

#include <opencv2/core/ocl.hpp>

#include <chrono>

namespace lvk
{

//---------------------------------------------------------------------------------------------------------------------

	DeblockingFilter::DeblockingFilter(DeblockingFilterSettings settings)
		: VideoFilter("Deblocking Filter")
	{
        configure(settings);
	}

//---------------------------------------------------------------------------------------------------------------------

	cv::UMat previous_frame;
	double duplicate_frame_count = 0.;
	uint64 frame_count = 0;
	double frametime = 0.;
	uint64 framerate_count = 0;
	auto start = std::chrono::steady_clock::now();
	int64 elapsed_seconds = 0;
	double average = 0.0;
	int count = 0;

    void DeblockingFilter::filter(
        Frame&& input,
        Frame& output,
        Stopwatch& timer,
        const bool debug
    )
	{
        LVK_ASSERT(!input.is_empty());

        if (debug)
		{ 
			cv::ocl::finish();
			timer.start();
		}

		if (input.data.empty())
			return;

		// first frame after starting filter is empty
		if (previous_frame.empty())
		{
			input.data.copyTo(previous_frame);
			return;
		}

		// if somehow the frame is empty, do nothing
		if (input.data.empty() || previous_frame.empty())
			return;

		cv::UMat current_frame, difference;
		input.data.copyTo(current_frame);

		cv::extractChannel(previous_frame, previous_frame, 0);
		cv::extractChannel(current_frame, current_frame, 0);
		cv::absdiff(previous_frame, current_frame, difference);

		double noise_filter = 0.;
		if (noise_filter > 0)
		{
			// filter for capture card with video noise
			cv::threshold(difference, difference, (double)noise_filter, 255, cv::THRESH_BINARY);
		}
		if (cv::countNonZero(difference) == 0)
			duplicate_frame_count++;
		else
		{
			// hardcoded timebase
			// todo: get from video settings
			frametime = (1000. / 60.) * (1 + duplicate_frame_count);
			framerate_count++;
			frame_count++;
			duplicate_frame_count = 0;
		}

		//cv::multiply(difference, difference, difference, 5);
		//cv::imshow("diff and noise", difference);

		// there's a better way to do this but I have no idea how
		auto end = std::chrono::steady_clock::now();
		elapsed_seconds = duration_cast<std::chrono::seconds>(end - start).count();
		if (elapsed_seconds >= 1)
		{
			average = static_cast<double>(framerate_count) / elapsed_seconds;
			start = end;
			framerate_count = 0;
		}

		if (debug)
		{
			cv::putText(
				input.data,
				cv::format("frametime %06.3lf duplicate %06.3lf frames %04llu number %llu average %06.3lf", frametime, duplicate_frame_count, frame_count, framerate_count, average),
				cv::Point(10, 80),
				cv::FONT_HERSHEY_SIMPLEX,
				1.0,
				cv::Scalar(149, 43, 21),
				2.0);
		}

		current_frame.copyTo(previous_frame);
		if (debug)
		{
			cv::ocl::finish();
			timer.stop();
		}
	}

//---------------------------------------------------------------------------------------------------------------------

	void lvk::DeblockingFilter::configure(const DeblockingFilterSettings& settings)
	{
		LVK_ASSERT(settings.block_size > 0);
		LVK_ASSERT(settings.filter_size >= 3);
		LVK_ASSERT(settings.filter_size % 2 == 1);
		LVK_ASSERT(settings.detection_levels > 0);
		LVK_ASSERT(settings.filter_scaling > 1.0f);

		m_Settings = settings;
	}

//---------------------------------------------------------------------------------------------------------------------

}
