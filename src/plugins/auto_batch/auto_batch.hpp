// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

///////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cpp_interfaces/impl/ie_executable_network_thread_safe_default.hpp"
#include "cpp_interfaces/impl/ie_infer_async_request_thread_safe_default.hpp"
#include "cpp_interfaces/interface/ie_iplugin_internal.hpp"
#include "ie_metric_helpers.hpp"
#include "threading/ie_thread_safe_containers.hpp"

namespace AutoBatchPlugin {

using DeviceName = std::string;

struct DeviceInformation {
    DeviceName deviceName;
    std::map<std::string, std::string> config;
    int batchForDevice;
};

class AutoBatchAsyncInferRequest;
class AutoBatchExecutableNetwork : public InferenceEngine::ExecutableNetworkThreadSafeDefault {
public:
    using Ptr = std::shared_ptr<AutoBatchExecutableNetwork>;
    struct WorkerInferRequest {
        using Ptr = std::shared_ptr<WorkerInferRequest>;
        InferenceEngine::SoIInferRequestInternal _inferRequestBatched;
        int _batchSize;
        InferenceEngine::ThreadSafeQueueWithSize<std::pair<AutoBatchAsyncInferRequest*, InferenceEngine::Task>> _tasks;
        std::vector<InferenceEngine::Task> _completionTasks;
        std::thread _thread;
        std::condition_variable _cond;
        std::mutex _mutex;
        std::exception_ptr _exceptionPtr;
    };

    explicit AutoBatchExecutableNetwork(
        const InferenceEngine::SoExecutableNetworkInternal& networkForDevice,
        const InferenceEngine::SoExecutableNetworkInternal& networkForDeviceWithoutBatch,
        const DeviceInformation& networkDevices,
        const std::unordered_map<std::string, InferenceEngine::Parameter>& config,
        const bool needPerfCounters = false);

    void SetConfig(const std::map<std::string, InferenceEngine::Parameter>& config) override;
    InferenceEngine::Parameter GetConfig(const std::string& name) const override;
    InferenceEngine::Parameter GetMetric(const std::string& name) const override;
    InferenceEngine::IInferRequestInternal::Ptr CreateInferRequest() override;
    InferenceEngine::IInferRequestInternal::Ptr CreateInferRequestImpl(
        InferenceEngine::InputsDataMap networkInputs,
        InferenceEngine::OutputsDataMap networkOutputs) override;
    InferenceEngine::IInferRequestInternal::Ptr CreateInferRequestImpl(
        const std::vector<std::shared_ptr<const ov::Node>>& inputs,
        const std::vector<std::shared_ptr<const ov::Node>>& outputs) override;
    std::shared_ptr<InferenceEngine::RemoteContext> GetContext() const override;
    std::shared_ptr<ngraph::Function> GetExecGraphInfo() override;
    virtual ~AutoBatchExecutableNetwork();

protected:
    static unsigned int ParseTimeoutValue(const std::string&);
    std::atomic_bool _terminate = {false};
    DeviceInformation _device;
    InferenceEngine::SoExecutableNetworkInternal _network;
    InferenceEngine::SoExecutableNetworkInternal _networkWithoutBatch;

    std::pair<WorkerInferRequest&, int> GetWorkerInferRequest();
    std::vector<WorkerInferRequest::Ptr> _workerRequests;
    std::mutex _workerRequestsMutex;

    std::unordered_map<std::string, InferenceEngine::Parameter> _config;
    bool _needPerfCounters = false;
    std::atomic_size_t _numRequestsCreated = {0};
    std::atomic_int _timeOut = {0};  // in ms
};

class AutoBatchInferRequest : public InferenceEngine::IInferRequestInternal {
public:
    using Ptr = std::shared_ptr<AutoBatchInferRequest>;
    explicit AutoBatchInferRequest(const InferenceEngine::InputsDataMap& networkInputs,
                                   const InferenceEngine::OutputsDataMap& networkOutputs,
                                   AutoBatchExecutableNetwork::WorkerInferRequest& workerRequestPtr,
                                   int batch_id,
                                   int num_batch,
                                   bool _needPerfCounters = false);
    explicit AutoBatchInferRequest(const std::vector<std::shared_ptr<const ov::Node>>& inputs,
                                   const std::vector<std::shared_ptr<const ov::Node>>& outputs,
                                   AutoBatchExecutableNetwork::WorkerInferRequest& workerRequestPtr,
                                   int batch_id,
                                   int num_batch,
                                   bool _needPerfCounters = false);

    std::map<std::string, InferenceEngine::InferenceEngineProfileInfo> GetPerformanceCounts() const override;
    // Batch-Device impl specific: sets the data (blobs from the device request to the batched device request)
    void SetBlobsToAnotherRequest(InferenceEngine::SoIInferRequestInternal& req);
    void CopyInputsIfNeeded();
    void CopyOutputsIfNeeded();
    AutoBatchExecutableNetwork::WorkerInferRequest& _myBatchedRequestWrapper;
    std::exception_ptr _exceptionPtr;
    enum eExecutionFlavor : uint8_t {
        NOT_EXECUTED,
        BATCH_EXECUTED,
        TIMEOUT_EXECUTED
    } _wasBatchedRequestUsed = eExecutionFlavor::NOT_EXECUTED;
    std::map<std::string, InferenceEngine::InferenceEngineProfileInfo> _perfMap;

protected:
    bool _needPerfCounters = false;
    void CopyBlobIfNeeded(InferenceEngine::Blob::CPtr src, InferenceEngine::Blob::Ptr dst, bool bInput);
    void ShareBlobsWithBatchRequest();
    size_t _batchId;
    size_t _batchSize;
};

class AutoBatchAsyncInferRequest : public InferenceEngine::AsyncInferRequestThreadSafeDefault {
public:
    using Ptr = std::shared_ptr<AutoBatchAsyncInferRequest>;

    explicit AutoBatchAsyncInferRequest(const AutoBatchInferRequest::Ptr& inferRequest,
                                        const bool needPerfCounters,
                                        InferenceEngine::SoIInferRequestInternal& inferRequestWithoutBatch,
                                        const InferenceEngine::ITaskExecutor::Ptr& callbackExecutor);
    void Infer_ThreadUnsafe() override;
    virtual ~AutoBatchAsyncInferRequest();

    InferenceEngine::SoIInferRequestInternal _inferRequestWithoutBatch;
    AutoBatchInferRequest::Ptr _inferRequest;
};

class AutoBatchInferencePlugin : public InferenceEngine::IInferencePlugin {
public:
    AutoBatchInferencePlugin();
    virtual ~AutoBatchInferencePlugin() = default;
    InferenceEngine::IExecutableNetworkInternal::Ptr LoadExeNetworkImpl(
        const InferenceEngine::CNNNetwork& network,
        const std::map<std::string, std::string>& config) override;
    InferenceEngine::IExecutableNetworkInternal::Ptr LoadExeNetworkImpl(
        const InferenceEngine::CNNNetwork& network,
        const std::shared_ptr<InferenceEngine::RemoteContext>& context,
        const std::map<std::string, std::string>& config) override;

    void SetConfig(const std::map<std::string, std::string>& config) override;
    void CheckConfig(const std::map<std::string, std::string>& config);

    InferenceEngine::Parameter GetConfig(
        const std::string& name,
        const std::map<std::string, InferenceEngine::Parameter>& options) const override;
    InferenceEngine::QueryNetworkResult QueryNetwork(const InferenceEngine::CNNNetwork& network,
                                                     const std::map<std::string, std::string>& config) const override;
    InferenceEngine::Parameter GetMetric(
        const std::string& name,
        const std::map<std::string, InferenceEngine::Parameter>& options) const override;
    InferenceEngine::RemoteContext::Ptr CreateContext(const InferenceEngine::ParamMap&) override;

protected:
    DeviceInformation ParseMetaDevice(const std::string& devicesBatchCfg,
                                      const std::map<std::string, std::string>& config) const;

    std::map<std::string, std::string> GetSupportedConfig(const std::map<std::string, std::string>& config,
                                                          const DeviceName& deviceName) const;
    static DeviceInformation ParseBatchDevice(const std::string& deviceWithBatch);

    InferenceEngine::IExecutableNetworkInternal::Ptr LoadNetworkImpl(
        const InferenceEngine::CNNNetwork& network,
        const std::shared_ptr<InferenceEngine::RemoteContext> context,
        const std::map<std::string, std::string>& config);
};

}  // namespace AutoBatchPlugin
