#pragma once

#include "measurement_types.h"

class AverageAccumulator {
 public:
  void addSample(const AccelSample &sample);

  bool hasSamples() const;

  AverageResult takeResult(
      uint32_t timestampMs,
      uint32_t overruns
  );

  void reset();

 private:
  int64_t accX_ = 0;
  int64_t accY_ = 0;
  int64_t accZ_ = 0;

  uint32_t sampleCount_ = 0;
};