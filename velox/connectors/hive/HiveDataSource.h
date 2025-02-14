/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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
#pragma once

#include "velox/common/io/IoStatistics.h"
#include "velox/connectors/Connector.h"
#include "velox/connectors/hive/FileHandle.h"
#include "velox/connectors/hive/HiveConnectorSplit.h"
#include "velox/connectors/hive/SplitReader.h"
#include "velox/connectors/hive/TableHandle.h"
#include "velox/dwio/common/BufferedInput.h"
#include "velox/dwio/common/Reader.h"
#include "velox/dwio/common/ScanSpec.h"
#include "velox/exec/OperatorUtils.h"
#include "velox/expression/Expr.h"

namespace facebook::velox::connector::hive {

class HiveDataSource : public DataSource {
 public:
  HiveDataSource(
      const RowTypePtr& outputType,
      const std::shared_ptr<connector::ConnectorTableHandle>& tableHandle,
      const std::unordered_map<
          std::string,
          std::shared_ptr<connector::ColumnHandle>>& columnHandles,
      FileHandleFactory* fileHandleFactory,
      core::ExpressionEvaluator* expressionEvaluator,
      cache::AsyncDataCache* cache,
      const std::string& scanId,
      folly::Executor* executor,
      const dwio::common::ReaderOptions& options);

  void addSplit(std::shared_ptr<ConnectorSplit> split) override;

  std::optional<RowVectorPtr> next(uint64_t size, velox::ContinueFuture& future)
      override;

  void addDynamicFilter(
      column_index_t outputChannel,
      const std::shared_ptr<common::Filter>& filter) override;

  uint64_t getCompletedBytes() override {
    return ioStats_->rawBytesRead();
  }

  uint64_t getCompletedRows() override {
    return completedRows_;
  }

  std::unordered_map<std::string, RuntimeCounter> runtimeStats() override;

  bool allPrefetchIssued() const override {
    return splitReader_ && splitReader_->allPrefetchIssued();
  }

  void setFromDataSource(std::unique_ptr<DataSource> sourceUnique) override;

  int64_t estimatedRowSize() override;

  // Internal API, made public to be accessible in unit tests.  Do not use in
  // other places.
  static std::shared_ptr<common::ScanSpec> makeScanSpec(
      const RowTypePtr& rowType,
      const folly::F14FastMap<
          std::string,
          std::vector<const common::Subfield*>>& outputSubfields,
      const SubfieldFilters& filters,
      const RowTypePtr& dataColumns,
      memory::MemoryPool* pool);

  // Internal API, made public to be accessible in unit tests.  Do not use in
  // other places.
  static core::TypedExprPtr extractFiltersFromRemainingFilter(
      const core::TypedExprPtr& expr,
      core::ExpressionEvaluator* evaluator,
      bool negated,
      SubfieldFilters& filters);

 protected:
  virtual std::unique_ptr<SplitReader> createSplitReader();

  std::unique_ptr<dwio::common::BufferedInput> createBufferedInput(
      const FileHandle&,
      const dwio::common::ReaderOptions&);

  std::shared_ptr<HiveConnectorSplit> split_;
  FileHandleFactory* fileHandleFactory_;
  dwio::common::ReaderOptions readerOpts_;
  std::shared_ptr<common::ScanSpec> scanSpec_;
  memory::MemoryPool* pool_;
  VectorPtr output_;
  std::unique_ptr<SplitReader> splitReader_;

  // Output type from file reader.  This is different from outputType_ that it
  // contains column names before assignment, and columns that only used in
  // remaining filter.
  RowTypePtr readerOutputType_;

  // Column handles for the partition key columns keyed on partition key column
  // name.
  std::unordered_map<std::string, std::shared_ptr<HiveColumnHandle>>
      partitionKeys_;

 private:
  // Evaluates remainingFilter_ on the specified vector. Returns number of rows
  // passed. Populates filterEvalCtx_.selectedIndices and selectedBits if only
  // some rows passed the filter. If none or all rows passed
  // filterEvalCtx_.selectedIndices and selectedBits are not updated.
  vector_size_t evaluateRemainingFilter(RowVectorPtr& rowVector);

  // Clear split_ after split has been fully processed.  Keep readers around to
  // hold adaptation.
  void resetSplit();

  void parseSerdeParameters(
      const std::unordered_map<std::string, std::string>& serdeParameters);

  const RowVectorPtr& getEmptyOutput() {
    if (!emptyOutput_) {
      emptyOutput_ = RowVector::createEmpty(outputType_, pool_);
    }
    return emptyOutput_;
  }

  std::shared_ptr<HiveTableHandle> hiveTableHandle_;

  // The row type for the data source output, not including filter only columns
  const RowTypePtr outputType_;
  std::shared_ptr<io::IoStatistics> ioStats_;
  std::shared_ptr<common::MetadataFilter> metadataFilter_;
  std::unique_ptr<exec::ExprSet> remainingFilterExprSet_;
  RowVectorPtr emptyOutput_;
  dwio::common::RuntimeStatistics runtimeStats_;
  core::ExpressionEvaluator* expressionEvaluator_;
  uint64_t completedRows_ = 0;

  // Reusable memory for remaining filter evaluation.
  VectorPtr filterResult_;
  SelectivityVector filterRows_;
  exec::FilterEvalCtx filterEvalCtx_;

  cache::AsyncDataCache* const cache_{nullptr};
  const std::string& scanId_;
  folly::Executor* executor_;
};

} // namespace facebook::velox::connector::hive
