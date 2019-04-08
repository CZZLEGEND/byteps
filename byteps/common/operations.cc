// Copyright 2019 ByteDance Inc. or its affiliates. All Rights Reserved.
// Copyright 2016 The TensorFlow Authors. All Rights Reserved.
// Modifications copyright (C) 2019 Uber Technologies, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// =============================================================================

#include <cstring>
#include <memory>
#include <thread>
#include <chrono>

#include "global.h"
#include "operations.h"
#include "logging.h"

namespace byteps {
namespace common {

bool RunPushLoopOnce() {
    auto q = BytePSGlobal::GetScheduledQueue(PUSH);
    while (q->pendingSize() > 0) {
        auto task = q->getTask();
        task->callback(Status::OK());
        BPS_LOG(TRACE) << "Finish pushing tensor: " << task->tensor_name;
    }
    return true;
}

bool RunPullLoopOnce() {
    auto q = BytePSGlobal::GetScheduledQueue(PULL);
    while (q->pendingSize() > 0) {
        auto task = q->getTask();
        task->callback(Status::OK());
        BPS_LOG(TRACE) << "Finish pulling tensor: " << task->tensor_name;
    }
    return true;
}

void PushLoop() {
    while (RunPushLoopOnce() && !BytePSGlobal::ShouldShutdown()) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(1000));
    }
}

void PullLoop() {
    while (RunPullLoopOnce() && !BytePSGlobal::ShouldShutdown()) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(1000));
    }
}

extern "C" {

void byteps_init(int rank, int local_rank, int size, int local_size) {
    LoopFunction func[ThreadNum] = {PushLoop, PullLoop};
    BytePSGlobal::Init(rank, local_rank, size, local_size, func);
    return;
}

void byteps_shutdown() {
    BytePSGlobal::Shutdown();
    BPS_LOG(TRACE) << "BytePS is shutdown.";
    return;
}

int byteps_rank() {
    return BytePSGlobal::GetRank();
}

int byteps_local_rank() {
    return BytePSGlobal::GetLocalRank();
}

int byteps_size() {
    return BytePSGlobal::GetSize();
}

int byteps_local_size() {
    return BytePSGlobal::GetLocalSize();
}

} // extern "C"

Status CheckInitialized() {
    return BytePSGlobal::CheckInit();
}

Status EnqueueTensorPush(std::shared_ptr<OpContext> context,
                              std::shared_ptr<Tensor> input,
                              std::shared_ptr<ReadyEvent> ready_event,
                              const std::string name, const int device,
                              const int priority, const int version,
                              StatusCallback callback) {
    std::shared_ptr<TensorTableEntry> e(new TensorTableEntry);
    e->tensor_name = name;
    e->context = context;
    e->tensor = input;
    e->output = NULL;
    e->ready_event = ready_event;
    e->device = device;
    e->priority = priority;
    e->version = version;
    e->callback = callback;

    BytePSGlobal::GetScheduledQueue(PUSH)->addTask(e);
    BPS_LOG(TRACE) << "EnqueueTensorPush: " << e->tensor_name;
    return Status::OK();
}

Status EnqueueTensorPull(std::shared_ptr<OpContext> context,
                              std::shared_ptr<Tensor> output,
                              std::shared_ptr<ReadyEvent> ready_event,
                              const std::string name, const int device,
                              const int priority, const int version,
                              StatusCallback callback) {
    std::shared_ptr<TensorTableEntry> e(new TensorTableEntry);
    e->tensor_name = name;
    e->context = context;
    e->tensor = NULL;
    e->output = output;
    e->ready_event = ready_event;
    e->device = device;
    e->priority = priority;
    e->version = version;
    e->callback = callback;

    BytePSGlobal::GetScheduledQueue(PULL)->addTask(e);
    BPS_LOG(TRACE) << "EnqueueTensorPull: " << e->tensor_name;
    return Status::OK();
}


} // namespace common
} // namespace byteps