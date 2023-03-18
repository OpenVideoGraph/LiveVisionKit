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

#include <obs-frontend-api.h>

#define str(s) #s
#define xstr(s) str(s)

namespace lvk
{

//---------------------------------------------------------------------------------------------------------------------

	DeblockingFilter::DeblockingFilter(DeblockingFilterSettings settings)
		: VideoFilter("Deblocking Filter")
	{
        configure(settings);
	}

//---------------------------------------------------------------------------------------------------------------------

	std::vector<uint32_t> fps_list(60, 0);

    void DeblockingFilter::filter(
        Frame&& input,
        Frame& output,
        Stopwatch& timer,
        const bool debug
    )
	{
        LVK_ASSERT(!input.is_empty());

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
		int tear_count = 0; // multiple for future fcat
		cv::UMat current_frame, difference, g_previous_frame, g_current_frame;
		input.data.copyTo(current_frame);
		cv::extractChannel(previous_frame, g_previous_frame, 0);
		cv::extractChannel(current_frame, g_current_frame, 0);
		cv::absdiff(g_previous_frame, g_current_frame, difference);
		double timebase = 1000. / (double)m_Settings.refresh_rate;
		double noise_filter = (double)m_Settings.noise_level;
		bool b_noise_filter = false;
		bool b_duplicate_frame = false;
		double tear_pos = 0.;
		double tear_height = 0.;
		bool b_is_recording = obs_frontend_recording_active();
		old_fps_list_size = fps_list.size();
		new_fps_list_size = m_Settings.refresh_rate;
		if (old_fps_list_size != new_fps_list_size)
		{
			cv::putText(
				input.data,
				cv::format("fps_list size mismatch (%lli/%lli)!!", old_fps_list_size, new_fps_list_size),
				cv::Point(10, 120),
				cv::FONT_HERSHEY_SIMPLEX,
				1.0,
				cv::Scalar(149, 43, 21),
				2.0);
			fps_list.resize(new_fps_list_size);
			return;
		}
		if (noise_filter > 0)
		{
			// filter for devices with video noise
			cv::threshold(difference, difference, (double)noise_filter, 255, cv::THRESH_BINARY);
			b_noise_filter = true;
		}
		cv::multiply(difference, difference, difference, 10);
		if (cv::countNonZero(difference) == 0)
		{
			duplicate_frame_count++;
			frametime = timebase * (1 + duplicate_frame_count);
			fps_list.erase(fps_list.begin());
			fps_list.push_back(0);
			b_duplicate_frame = true;
		}
		else
		{
			// hardcoded timebase
			// todo: get from video settings
			frametime = timebase * (1 + duplicate_frame_count);
			frame_count++;
			fps_list.erase(fps_list.begin());
			fps_list.push_back(1);
			b_duplicate_frame = false;
			duplicate_frame_count = 0;
		}

		framerate = std::accumulate(fps_list.begin(), fps_list.end(), 0.0);

			/*
			int tear_top = 0;
			int tear_center = 0;
			int tear_bottom = 0;
			if (tear_count > 0)
			{
				// 30fps only, 60fps is different.
				// write another path or try to make this path work for both?
				// also try to get processing time below 5ms
				// FIXME: make these testcases pass: https://drive.google.com/drive/folders/1TtAu5kIpfV8cJ8KVTAN0GtWx1esSO6mW
				cv::multiply(difference, difference, difference, 10);
				std::vector<std::vector<cv::Point>> contours;
				findContours(difference, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
				for (const auto& contour : contours)
				{
					cv::Rect rect = boundingRect(contour);
					if (rect.width > 10 && rect.height > 10)
					{
						tear_center = rect.y + rect.height / 2;
						tear_top = tear_center - rect.height / 2;
						tear_bottom = tear_center + rect.height / 2;
						tear_center = tear_bottom - tear_top;
					}
				}
			}
			// draw line is slow
			if (tear_top > 2 && tear_center > 2 && tear_bottom > 2)
				cv::line(input.data, cv::Point(0, tear_top), cv::Point(input.data.cols, tear_top), cv::Scalar(0, 0, 255), 2);
			//cv::line(input.data, cv::Point(0, tear_bottom), cv::Point(input.data.cols, tear_bottom), cv::Scalar(255, 0, 0), 2);
			*/

		if (debug)
		{
			cv::putText(
				input.data,
				cv::format("%04llu,%04llu,%04llf,%02i,%04llf,%04llf,%lli,%lli",
					video_frame_count,
					frame_count,
					frametime,
					b_duplicate_frame,
					duplicate_frame_count,
					framerate,
					old_fps_list_size,
					new_fps_list_size),
				cv::Point(10, 80),
				cv::FONT_HERSHEY_SIMPLEX,
				1.0,
				cv::Scalar(149, 43, 21),
				2.0);
		}
		else if (m_Settings.overlay_video)
		{
			cv::putText(
				input.data,
				cv::format("FPS: %.2llf Frametime: %.2llf",
					framerate,
					frametime),
				cv::Point(10, 80),
				cv::FONT_HERSHEY_SIMPLEX,
				1.0,
				cv::Scalar(149, 43, 21),
				2.0);
		}

		if (b_is_recording && !stats_file_opened)
		{
			video_frame_count = 0;
			frame_count = 0;
			frame_stats.open("C:\\test\\test.csv");
			std::string data =
				str(video_frame_count) ","
				str(frame_count) ","
				str(frametime) ","
				str(b_duplicate_frame) ","
				str(duplicate_frame_count) ","
				str(tear_pos) ","
				str(tear_height) "\n";
			frame_stats << data;
			stats_file_opened = true;
			frame_count = 0;
		}
		if (b_is_recording && stats_file_opened)
		{
			std::string data =
				cv::format("%llu,%llu,%llf,%i,%llf,%llf,%llf\n",
				video_frame_count,
				frame_count,
				frametime,
				b_duplicate_frame,
				duplicate_frame_count,
				tear_pos,
				tear_height);
			frame_stats << data;
		}
		if (!b_is_recording && stats_file_opened)
		{
			frame_stats.close();
			stats_file_opened = false;
		}
		video_frame_count++;
		current_frame.copyTo(previous_frame);
	}

//---------------------------------------------------------------------------------------------------------------------

	void lvk::DeblockingFilter::configure(const DeblockingFilterSettings& settings)
	{
		m_Settings = settings;
	}

//---------------------------------------------------------------------------------------------------------------------

}
