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
        this->configure(settings);
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
	double timebase = 1000. / 60.;

    void DeblockingFilter::filter(
        const Frame& input,
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

		// if user is changing source resolution, make it match
		if (input.data.rows != previous_frame.rows)
		{
			// just to make sure, run the comparison next frame
			input.data.copyTo(previous_frame);
			return;
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

		cv::UMat current_frame, difference, g_previous_frame, g_current_frame;
		input.data.copyTo(current_frame);

		cv::extractChannel(previous_frame, g_previous_frame, 0);
		cv::extractChannel(current_frame, g_current_frame, 0);
		cv::absdiff(g_previous_frame, g_current_frame, difference);
		// previous_frame.copyTo(input.data); // not the way to do it, we should never have to show the previous frame in no vsync mode

		double noise_filter = (double)m_Settings.detection_levels;
		if (noise_filter > 0)
		{
			// filter for capture card with video noise
			cv::threshold(difference, difference, (double)noise_filter, 255, cv::THRESH_BINARY);
			cv::putText(
				input.data,
				"threshold active!",
				cv::Point(10, 120),
				cv::FONT_HERSHEY_SIMPLEX,
				1.0,
				cv::Scalar(149, 43, 21),
				2.0);
		}
		if (cv::countNonZero(difference) == 0)
		{
			duplicate_frame_count++;
			frametime = timebase * (1 + duplicate_frame_count);
		}
		else
		{
			// hardcoded timebase
			// todo: get from video settings
			frametime = timebase * (1 + duplicate_frame_count);
			framerate_count++;
			frame_count++;
			duplicate_frame_count = 0;
		}

		//cv::imshow("diff and noise", difference);
		//difference.copyTo(input.data);
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
			/*
			// this lags one frame behind
			cv::multiply(difference, difference, difference, 10);
			std::vector<std::vector<cv::Point>> contours;
			findContours(difference, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
			int tear_pos = 0;
			for (const auto& contour : contours) {
				cv::Rect rect = boundingRect(contour);
				if (rect.width > 10 && rect.height > 10) {
					tear_pos = rect.y + rect.height;
				}
			}
			// draw line is slow
			cv::line(input.data, cv::Point(0, tear_pos), cv::Point(input.data.cols, tear_pos), cv::Scalar(0, 0, 255), 2);
			*/
			cv::putText(
				input.data,
				cv::format("frametime %06.3lf duplicate %06.3lf frames %04llu number %llu average %06.3lf noise %06.3lf", frametime, duplicate_frame_count, frame_count, framerate_count, average, noise_filter),
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
		//LVK_ASSERT(settings.detection_levels > 0);
		LVK_ASSERT(settings.filter_scaling > 1.0f);

		m_Settings = settings;
	}

//---------------------------------------------------------------------------------------------------------------------

}
