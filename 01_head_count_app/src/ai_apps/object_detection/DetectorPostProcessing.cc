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
#include "DetectorPostProcessing.hpp"
#include "PlatformMath.hpp"

#include <cmath>

namespace arm {
namespace app {

DetectorPostProcess::DetectorPostProcess(
        TfLiteTensor* modelOutput0,
        std::vector<object_detection::DetectionResult>& results,
        const object_detection::PostProcessParams& postProcessParams)
        :   m_outputTensor0{modelOutput0},
            m_results{results},
            m_postProcessParams{postProcessParams}
{
    /* Init PostProcessing */
    this->m_net = object_detection::Network{
        .inputWidth  = postProcessParams.inputImgCols,
        .inputHeight = postProcessParams.inputImgRows,
        .numClasses  = postProcessParams.numClasses,
        .branches =
            {object_detection::Branch{
                                      .modelOutput = this->m_outputTensor0->data.int8,
                                      .scale       = (static_cast<TfLiteAffineQuantization*>(
                                                    this->m_outputTensor0->quantization.params))
                                                   ->scale->data[0],
                                      .zeroPoint = (static_cast<TfLiteAffineQuantization*>(
                                                        this->m_outputTensor0->quantization.params))
                                                       ->zero_point->data[0],
                                      .size = this->m_outputTensor0->bytes}
                                      },
        .topN = postProcessParams.topN};
    /* End init */
}

bool DetectorPostProcess::DoPostProcess()
{
    /* Start postprocessing */
    int originalImageWidth  = m_postProcessParams.originalImageSize;
    int originalImageHeight = m_postProcessParams.originalImageSize;

    std::forward_list<image::Detection> detections;
    GetNetworkBoxes(this->m_net, originalImageWidth, originalImageHeight, m_postProcessParams.threshold, detections);

    /* Do nms */
    CalculateNMS(detections, this->m_net.numClasses, this->m_postProcessParams.nms);

    for (auto& it: detections) {
        float xMin = it.bbox.x - it.bbox.w / 2.0f;
        float xMax = it.bbox.x + it.bbox.w / 2.0f;
        float yMin = it.bbox.y - it.bbox.h / 2.0f;
        float yMax = it.bbox.y + it.bbox.h / 2.0f;

        if (xMin < 0) {
            xMin = 0;
        }
        if (yMin < 0) {
            yMin = 0;
        }
        if (xMax > originalImageWidth) {
            xMax = originalImageWidth;
        }
        if (yMax > originalImageHeight) {
            yMax = originalImageHeight;
        }

        float boxX = xMin;
        float boxY = yMin;
        float boxWidth = xMax - xMin;
        float boxHeight = yMax - yMin;

        for (int j = 0; j < this->m_net.numClasses; ++j) {
            if (it.prob[j] > 0) {

                object_detection::DetectionResult tmpResult = {};
                tmpResult.m_normalisedVal = it.prob[j];
                tmpResult.m_x0 = boxX;
                tmpResult.m_y0 = boxY;
                tmpResult.m_w = boxWidth;
                tmpResult.m_h = boxHeight;

                this->m_results.push_back(tmpResult);
            }
        }
    }
    return true;
}

void DetectorPostProcess::InsertTopNDetections(std::forward_list<image::Detection>& detections, image::Detection& det)
{
    std::forward_list<image::Detection>::iterator it;
    std::forward_list<image::Detection>::iterator last_it;
    for ( it = detections.begin(); it != detections.end(); ++it ) {
        if(it->objectness > det.objectness)
            break;
        last_it = it;
    }
    if(it != detections.begin()) {
        detections.emplace_after(last_it, det);
        detections.pop_front();
    }
}

static float Sigmoid(float x)
{
    return 1.0f / (1.0f + exp(-x));
}

static float Tanh(float x)
{
    return 2.0f / (1.0f + exp(-2 * x)) - 1;
}

void DetectorPostProcess::GetNetworkBoxes(
        object_detection::Network& net,
        int imageWidth,
        int imageHeight,
        float threshold,
        std::forward_list<image::Detection>& detections)
{
    int numClasses = net.numClasses;
    int num = 0;
    auto det_objectness_comparator = [](image::Detection& pa, image::Detection& pb) {
        return pa.objectness < pb.objectness;
    };
    for (size_t i = 0; i < net.branches.size(); ++i) {
        int height   = m_postProcessParams.output_dim[0];
        int width    = m_postProcessParams.output_dim[1];
        int channel  = m_postProcessParams.output_dim[2];

        for (int idx = 0; idx < (height*width*channel); idx = idx+channel){
        	float obj_score = (static_cast<float>(net.branches[i].modelOutput[idx])
        	        				- net.branches[i].zeroPoint)
        	                        * net.branches[i].scale;
        	int category;
			float max_score = 0.0f;
			for (size_t i = 0; i < numClasses; i++)
			{
				float cls_score = (static_cast<float>(net.branches[i].modelOutput[idx+5])
						- net.branches[i].zeroPoint)
						* net.branches[i].scale;
				if (cls_score > max_score)
				{
					max_score = cls_score;
					category = i;
				}
			}
			float score = pow(max_score, 0.4) * pow(obj_score, 0.6);

			if(score > threshold) {
				image::Detection det;
				det.objectness = score;

				float x_offset = Tanh(
									 (static_cast<float>(net.branches[i].modelOutput[idx+1])
									  - net.branches[i].zeroPoint)
									  * net.branches[i].scale
									 );
				float y_offset = Tanh(
									 (static_cast<float>(net.branches[i].modelOutput[idx+2])
									  - net.branches[i].zeroPoint)
									  * net.branches[i].scale
									 );
				float box_width = Sigmoid(
									(static_cast<float>(net.branches[i].modelOutput[idx+3])
									 - net.branches[i].zeroPoint)
									 * net.branches[i].scale
									);
				float box_height = Sigmoid(
						(static_cast<float>(net.branches[i].modelOutput[idx+4])
						 - net.branches[i].zeroPoint)
						 * net.branches[i].scale
						);
				float cx = ((int)((idx/channel)%width) + x_offset) / width;
				float cy = ((int)(idx/(height*channel)) + y_offset) / height;

				det.bbox.x = (int)((cx) * imageWidth);
				det.bbox.y = (int)((cy) * imageHeight);
				det.bbox.w = box_width * imageWidth;
				det.bbox.h = box_height * imageHeight;

				det.prob.emplace_back(score);

				if (num < net.topN || net.topN <=0) {
					detections.emplace_front(det);
					num += 1;
				} else if (num == net.topN) {
					detections.sort(det_objectness_comparator);
					InsertTopNDetections(detections, det);
					num += 1;
				} else {
					InsertTopNDetections(detections, det);
				}
			}
        }
    }
    if(num > net.topN)
        num -=1;
}

} /* namespace app */
} /* namespace arm */
