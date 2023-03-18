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

#include "ADBFilter.hpp"

#include <util/platform.h>
#include <functional>

#include "Utility/Locale.hpp"

namespace lvk
{

//---------------------------------------------------------------------------------------------------------------------

	constexpr auto PROP_STRENGTH = "STRENGTH";
	constexpr auto STRENGTH_MAX = 10;
	constexpr auto STRENGTH_MIN = 0;
	constexpr auto STRENGTH_DEFAULT = 3;

	constexpr auto PROP_REFRESH_RATE = "REFRESH_RATE";
	constexpr auto REFRESH_RATE_MAX = 1000;
	constexpr auto REFRESH_RATE_MIN = 60;
	constexpr auto REFRESH_RATE_DEFAULT = 60;

	constexpr auto PROP_TEST_MODE = "TEST_MODE";
	constexpr auto TEST_MODE_DEFAULT = false;

	constexpr auto PROP_PROCESSING_MODE = "PROCESSING_MODE";
	constexpr auto PROCESSING_MODE_DEFAULT = false;

	constexpr auto PROP_OVERLAY_MODE = "OVERLAY_MODE";
	constexpr auto OVERLAY_MODE_DEFAULT = false;

	constexpr auto TIMING_THRESHOLD_MS = 3.0;
	constexpr auto TIMING_SAMPLES = 30;

//---------------------------------------------------------------------------------------------------------------------

	obs_properties_t* ADBFilter::Properties()
	{
		obs_properties_t* properties = obs_properties_create();

		obs_properties_add_int_slider(
			properties,
			PROP_STRENGTH,
			L("adb.strength"),
			STRENGTH_MIN,
			STRENGTH_MAX,
			1
		);

		obs_properties_add_int(
			properties,
			PROP_REFRESH_RATE,
			L("adb.refresh_rate"),
			REFRESH_RATE_MIN,
			REFRESH_RATE_MAX,
			60
		);

		obs_properties_add_bool(
			properties,
			PROP_PROCESSING_MODE,
			L("adb.processing_mode")
		);

		obs_properties_add_bool(
			properties,
			PROP_OVERLAY_MODE,
			L("adb.overlay")
		);

		obs_properties_add_bool(
			properties,
			PROP_TEST_MODE,
			L("f.testmode")
		);

		return properties;
	}

//---------------------------------------------------------------------------------------------------------------------

	void ADBFilter::LoadDefaults(obs_data_t* settings)
	{
		LVK_ASSERT(settings != nullptr);

		obs_data_set_default_int(settings, PROP_STRENGTH, STRENGTH_DEFAULT);
		obs_data_set_default_int(settings, PROP_REFRESH_RATE, REFRESH_RATE_DEFAULT);
		obs_data_set_default_bool(settings, PROP_PROCESSING_MODE, PROCESSING_MODE_DEFAULT);
		obs_data_set_default_bool(settings, PROP_OVERLAY_MODE, OVERLAY_MODE_DEFAULT);
		obs_data_set_default_bool(settings, PROP_TEST_MODE, TEST_MODE_DEFAULT);
	}

//---------------------------------------------------------------------------------------------------------------------

	void ADBFilter::configure(obs_data_t* settings)
	{
		LVK_ASSERT(settings != nullptr);

		const auto strength = obs_data_get_int(settings, PROP_STRENGTH);
		const auto rate = obs_data_get_int(settings, PROP_REFRESH_RATE);
		m_TestMode = obs_data_get_bool(settings, PROP_TEST_MODE);
		m_EnableProcessing = obs_data_get_bool(settings, PROP_PROCESSING_MODE);
		const auto overlay_bool = obs_data_get_bool(settings, PROP_OVERLAY_MODE);

		m_Filter.reconfigure([&](DeblockingFilterSettings& settings) {
			settings.noise_level = strength;
			settings.enable_processing = m_EnableProcessing;
			settings.overlay_video = overlay_bool;
			settings.refresh_rate = rate;
		});
	}

//---------------------------------------------------------------------------------------------------------------------

	ADBFilter::ADBFilter(obs_source_t* context)
		: VisionFilter(context),
		  m_Context(context),
		  m_Filter()
	{
		LVK_ASSERT(context != nullptr);

        m_Filter.set_timing_samples(TIMING_SAMPLES);
	}

//---------------------------------------------------------------------------------------------------------------------

	void ADBFilter::filter(FrameBuffer& frame)
	{
		if (m_EnableProcessing)
		{
			if (m_TestMode)
				m_Filter.process(frame, frame, true);
			else
				m_Filter.process(frame, frame, false);
		}
	}

//---------------------------------------------------------------------------------------------------------------------

	bool ADBFilter::validate() const
	{
		return m_Context != nullptr;
	}

//---------------------------------------------------------------------------------------------------------------------

}
