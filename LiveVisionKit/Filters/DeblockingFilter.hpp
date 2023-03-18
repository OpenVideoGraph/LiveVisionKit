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

#pragma once

#include "VideoFilter.hpp"

#include "Utility/Properties/Configurable.hpp"

#include <numeric>

#include <fstream>

namespace lvk
{

	struct DeblockingFilterSettings
	{
		bool overlay_video = false;
		bool enable_processing = false;
		size_t refresh_rate = 60; // Must be greater than 0
		uint32_t noise_level = 0; // Must be greater than 0
	};

	class DeblockingFilter final : public VideoFilter, public Configurable<DeblockingFilterSettings>
	{
	public:

		explicit DeblockingFilter(DeblockingFilterSettings settings = {});
		
		void configure(const DeblockingFilterSettings& settings) override;

	private:

		cv::UMat previous_frame;
		uint64_t frame_count = 0;
		uint64_t video_frame_count = 0;
		size_t old_fps_list_size = 0;
		size_t new_fps_list_size = 0;
		double duplicate_frame_count = 0.;
		double framerate = 0.;
		double frametime = 0.;
		bool stats_file_opened = false;
		std::ofstream frame_stats;

        // NOTE: Supports in-place operation.
        void filter(
            Frame&& input,
            Frame& output,
            Stopwatch& timer,
            const bool debug
        ) override;
	};

}