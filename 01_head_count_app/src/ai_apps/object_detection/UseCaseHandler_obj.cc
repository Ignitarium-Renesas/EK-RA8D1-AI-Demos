/*
 * SPDX-FileCopyrightText: Copyright 2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <UseCaseHandler_obj.hpp>
#include "FastestDetModel.hpp"
#include "UseCaseCommonUtils.hpp"
#include "DetectorPostProcessing.hpp"
#include "DetectorPreProcessing.hpp"
#include "log_macros.h"
#include "camera_layer.h"

extern "C" {
#include "timer.h"
void update_detection_result(signed short index, signed short  x, signed short  y, signed short  w, signed short  h);
}

#include <cinttypes>

extern uint8_t bsp_camera_out_buffer888[];
extern uint8_t bsp_det_model_ip_buffer888[];
extern uint8_t greyscale_feed_buff[];


#define IMAGE_DATA_SIZE  (110592U)
#define DST_HEIGHT 192
#define DST_WIDTH  192
#define SRC_HEIGHT 240
#define SRC_WIDTH  240
#define img_channel 3
#define crop_offset 40

extern "C" {
	extern uint64_t object_detection_inference_time;
}

/* Tracker */
uint8_t maxInactiveFrames = 3;
uint8_t minActiveFrames = 1;
float track_IOU_thresold = 0.3;

arm::app::Tracker tracker(maxInactiveFrames, track_IOU_thresold);

extern void nearestNeighborSampling(uint8_t* srcImage, uint8_t* dstImage, bool pad) 
{
    
    int f_dst_width;
    int f_dst_height;
    int incr_pointer = 0;
    if(pad)
    {
        int max_val = SRC_WIDTH>SRC_HEIGHT?SRC_WIDTH:SRC_HEIGHT;
        float scale = (float)DST_WIDTH/(float)max_val;
        f_dst_width = SRC_WIDTH*scale;
        f_dst_height = SRC_HEIGHT*scale;

        int pad_w_offset = f_dst_width<DST_WIDTH?(DST_WIDTH-f_dst_width)/2:0;
        int pad_h_offset =  f_dst_height<DST_HEIGHT?(DST_HEIGHT-f_dst_height)/2:0;

        incr_pointer = pad_h_offset*3*DST_WIDTH + 3*pad_w_offset;
    }

    else
    {
        f_dst_width = DST_WIDTH;
        f_dst_height = DST_HEIGHT;
    }
    
    for(int y = 0; y < f_dst_height; y++)
    {
        for (int x = 0; x < f_dst_width; x++)
        {
            int srcX = (int)(x * ((float)320 / f_dst_width));
            int srcY = (int)(y * ((float)240 / f_dst_height));
            int srcIndex = (srcY * 320 + srcX) * 3;
            int dstIndex = (y * DST_WIDTH + x) * 3 + incr_pointer;

            dstImage[dstIndex]     = srcImage[srcIndex];
            dstImage[dstIndex + 1] = srcImage[srcIndex + 1];
            dstImage[dstIndex + 2] = srcImage[srcIndex + 2];
        }
    }

}

namespace arm {
namespace app {
    /**
     * @brief           Presents inference results along using the data presentation
     *                  object.
     * @param[in]       results            Vector of detection results to be displayed.
     * @return          true if successful, false otherwise.
     **/
    static bool PresentInferenceResult(const std::vector<object_detection::DetectionResult>& results);

    /**
     * @brief           Draw boxes directly on the LCD for all detected objects.
     * @param[in]       results            Vector of detection results to be displayed.
     * @param[in]       imageStartX        X coordinate where the image starts on the LCD.
     * @param[in]       imageStartY        Y coordinate where the image starts on the LCD.
     * @param[in]       imgDownscaleFactor How much image has been downscaled on LCD.
     **/

    /* Object detection inference handler. */
    bool ObjectDetectionHandler(ApplicationContext& ctx, uint32_t imgIndex, bool runAll)
    {
//    	uint64_t t2 = get_timestamp();
        auto& model = ctx.Get<Model&>("model");
        
        if (!model.IsInited()) {
            error("Model is not initialized! Terminating processing.\n");
            return false;
        }

        TfLiteTensor* inputTensor = model.GetInputTensor(0);
        TfLiteTensor* outputTensor0 = model.GetOutputTensor(0);

        if (!inputTensor->dims) {
            error("Invalid input tensor dims\n");
            return false;
        } else if (inputTensor->dims->size < 3) {
            error("Input tensor dimension should be >= 3\n");
            return false;
        }

        TfLiteIntArray* inputShape = model.GetInputShape(0);

        const int inputImgCols = inputShape->data[FastestDetModel::ms_inputColsIdx];
        const int inputImgRows = inputShape->data[FastestDetModel::ms_inputRowsIdx];

        /* Set up pre and post-processing. */
        DetectorPreProcess preProcess = DetectorPreProcess(inputTensor, false, model.IsDataSigned());

        std::vector<object_detection::DetectionResult> results;
        
        arm::app::object_detection::PostProcessParams postProcessParams {
            inputImgRows, inputImgCols, object_detection::originalImageSize
        };
        DetectorPostProcess postProcess = DetectorPostProcess(outputTensor0,
                results, postProcessParams);
        {
            /* Ensure there are no results leftover from previous inference when running all. */
            results.clear();
                
              
            nearestNeighborSampling(bsp_camera_out_buffer888,
									bsp_det_model_ip_buffer888,
									false);
			const uint8_t* currImage = bsp_det_model_ip_buffer888;
        
            const size_t copySz = inputTensor->bytes < IMAGE_DATA_SIZE ?
                                inputTensor->bytes : IMAGE_DATA_SIZE;

            /* Run the pre-processing, inference and post-processing. */
            if (!preProcess.DoPreProcess(currImage, copySz)) {
                error("Pre-processing failed.");
                return false;
            }
            uint64_t t1 = get_timestamp();
            if (!RunInference(model)) {
                error("Inference failed.");
                return false;
            }
            object_detection_inference_time = get_timestamp() - t1;


            if (!postProcess.DoPostProcess()) {
                error("Post-processing failed.");
                return false;
            }
            tracker.update(results);

            if (!PresentInferenceResult(results)) {
                return false;
            }
//            e_printf("\r\nTotal inference time: %.1f ms", (get_timestamp() - t2)/1000.f);

        }
        return true;
    }

    static bool PresentInferenceResult(const std::vector<object_detection::DetectionResult>& results)
    {
    	for (uint32_t i = 0; i < results.size(); ++i) {
    		if(results[i].m_box_id_count >= minActiveFrames)
    			update_detection_result(i, results[i].m_x0, results[i].m_y0, results[i].m_w, results[i].m_h );
        }

        return true;
    }

//    static void CvtToGreyScale(uint8_t rgb_image, uint8_t gray_image)
//    {
//    	int width = 192;
//    	int height  =192;
//        for (int i = 0; i < height; i++) {
//            for (int j = 0; j < width; j++) {
//                // Get the RGB values of the current pixel
//                unsigned char r = rgb_image[3 * (i * width + j)];
//                unsigned char g = rgb_image[3 * (i * width + j) + 1];
//                unsigned char b = rgb_image[3 * (i * width + j) + 2];
//
//                // Convert RGB to grayscale using a weighted sum
//                gray_image[i * width + j] = 0.2126 * r + 0.7152 * g + 0.0722 * b;
//            }
//        }
//    }



} /* namespace app */
} /* namespace arm */
