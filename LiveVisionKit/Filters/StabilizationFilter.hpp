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
#include "Vision/Tracking/FrameTracker.hpp"
#include "Vision/Tracking/PathStabilizer.hpp"
#include "Utility/Properties/Configurable.hpp"

namespace lvk
{

	// TODO: FrameTracker settings are hidden, perhaps make it configurable?

	struct StabilizationSettings
	{
		size_t smoothing_frames = 10;
		bool stabilize_output = true;

		bool crop_output = false;
		float crop_proportion = 0.05f;

		bool auto_suppression = true;
		float suppression_threshold = 0.9f;
		float suppression_saturation_limit = 0.7f;
		float suppression_smoothing_rate = 0.05f;

		MotionModel motion_model = MotionModel::DYNAMIC;
	};

	// TODO: fix bad timing when using profile and debug due to stopwatch

	class StabilizationFilter : public VideoFilter, public Configurable<StabilizationSettings>
	{
	public:

		StabilizationFilter(const StabilizationSettings& settings = {});

		void process(const Frame& input, Frame& output, const bool debug = false) override;
		
		void configure(const StabilizationSettings& settings = {}) override;

		bool ready() const;

		void restart();

		// TODO: rename this
		void reset_tracking();

		float stability() const;

		uint32_t frame_delay() const;

		const cv::Rect& crop_region() const;

	private:

		Homography suppress(Homography& motion);

	private:
		float m_SuppressionFactor = 0.0f;

		FrameTracker m_FrameTracker;
		PathStabilizer m_Stabilizer;

		cv::UMat m_TrackingFrame{cv::UMatUsageFlags::USAGE_ALLOCATE_DEVICE_MEMORY};
	};

}