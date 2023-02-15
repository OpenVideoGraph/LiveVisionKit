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


namespace lvk
{

//---------------------------------------------------------------------------------------------------------------------

	DeblockingFilter::DeblockingFilter(DeblockingFilterSettings settings)
		: VideoFilter("Deblocking Filter")
	{
        this->configure(settings);
	}

//---------------------------------------------------------------------------------------------------------------------

    void DeblockingFilter::filter(
        const Frame& input,
        Frame& output,
        Stopwatch& timer,
        const bool debug
    )
	{
        LVK_ASSERT(!input.is_empty());

        if(debug) cv::ocl::finish();
        timer.start();
        if(debug) cv::ocl::finish();
        timer.stop();
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
