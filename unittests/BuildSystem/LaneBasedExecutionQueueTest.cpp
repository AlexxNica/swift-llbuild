//===- unittests/BuildSystem/LaneBasedExecutionQueueTest.cpp --------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2016 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://swift.org/LICENSE.txt for license information
// See http://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "llbuild/Basic/FileSystem.h"
#include "llbuild/BuildSystem/BuildExecutionQueue.h"
#include "TempDir.hpp"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"

#include "gtest/gtest.h"

#include <ctime>

using namespace llbuild;
using namespace llbuild::basic;
using namespace llbuild::buildsystem;

namespace {
  class DummyDelegate : public BuildExecutionQueueDelegate {
  public:
    DummyDelegate() {}

    virtual void commandJobStarted(Command* command) override {}
    virtual void commandJobFinished(Command* command) override {}
    virtual void commandProcessStarted(Command* command, ProcessHandle handle) override {}
    virtual void commandProcessHadError(Command* command, ProcessHandle handle, const Twine& message) override {}
    virtual void commandProcessHadOutput(Command* command, ProcessHandle handle, StringRef data) override {}
    virtual void commandProcessFinished(Command* command, ProcessHandle handle, int exitStatus) override {}
  };

  TEST(LaneBasedExecutionQueueTest, basic) {
    DummyDelegate delegate;
    std::unique_ptr<FileSystem> fs = createLocalFileSystem();
    TmpDir tempDir{"LaneBasedExecutionQueueTest"};
    std::string outputFile = tempDir.str() + "/yes-output.txt";
    auto queue = std::unique_ptr<BuildExecutionQueue>(createLaneBasedExecutionQueue(delegate, 2));

    auto fn = [&outputFile, &queue](QueueJobContext* context) {
      queue->executeShellCommand(context, "yes >" + outputFile);
    };

    queue->addJob(QueueJob((Command*)0x1, fn));

    // Busy wait until `outputFile` appears which indicates that `yes` is running
    time_t start = ::time(NULL);
    while (fs->getFileInfo(outputFile).isMissing()) {
      if (::time(NULL) > start + 5) {
         // We can't fail gracefully because the `LaneBasedExecutionQueue` will always wait for spawned processes to exit
        abort();
      }
    }

    queue->cancelAllJobs();
    queue.reset();
  }
    
}
