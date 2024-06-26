/* Copyright 2021 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_LITE_MICRO_TEST_HELPER_CUSTOM_OPS_H_
#define TENSORFLOW_LITE_MICRO_TEST_HELPER_CUSTOM_OPS_H_

#include <ai_apps/common/tensorflow/lite/c/common.h>
#include <ai_apps/common/tensorflow/lite/kernels/internal/compatibility.h>
#include <ai_apps/common/tensorflow/lite/kernels/internal/tensor_ctypes.h>
#include <ai_apps/common/tensorflow/lite/micro/all_ops_resolver.h>
#include <ai_apps/common/tensorflow/lite/micro/micro_utils.h>
#include <ai_apps/common/tensorflow/lite/portable_type_to_tflitetype.h>
#include <ai_apps/common/tensorflow/lite/schema/schema_generated.h>
#include <ai_apps/common/third_party/flatbuffers/include/flatbuffers/flatbuffers.h>  // from @flatbuffers
#include <cstdint>
#include <limits>


namespace tflite {
namespace testing {

class PackerOp {
 public:
  static const TfLiteRegistration* getRegistration();
  static TfLiteRegistration* GetMutableRegistration();
  static void* Init(TfLiteContext* context, const char* buffer, size_t length);
  static void Free(TfLiteContext* context, void* buffer);
  static TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node);
  static TfLiteStatus Invoke(TfLiteContext* context, TfLiteNode* node);

 private:
  static bool freed_;
};

}  // namespace testing
}  // namespace tflite

#endif  // TENSORFLOW_LITE_MICRO_TEST_HELPER_CUSTOM_OPS_H_
