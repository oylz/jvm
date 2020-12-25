#ifndef _JVM_INNERH_
#define _JVM_INNERH_

#ifdef JVM_INNER
#include "precompiled.hpp"
#include "classfile/systemDictionary.hpp"
#include "classfile/vmSymbols.hpp"
#include "interpreter/bytecodeStream.hpp"
#include "interpreter/interpreter.hpp"
#include "jvmtifiles/jvmtiEnv.hpp"
#include "memory/resourceArea.hpp"
#include "memory/universe.inline.hpp"
#include "oops/instanceKlass.hpp"
#include "prims/jniCheck.hpp"
#include "prims/jvm_misc.hpp"
#include "prims/jvmtiAgentThread.hpp"
#include "prims/jvmtiClassFileReconstituter.hpp"
#include "prims/jvmtiCodeBlobEvents.hpp"
#include "prims/jvmtiExtensions.hpp"
#include "prims/jvmtiGetLoadedClasses.hpp"
#include "prims/jvmtiImpl.hpp"
#include "prims/jvmtiManageCapabilities.hpp"
#include "prims/jvmtiRawMonitor.hpp"
#include "prims/jvmtiRedefineClasses.hpp"
#include "prims/jvmtiTagMap.hpp"
#include "prims/jvmtiThreadState.inline.hpp"
#include "prims/jvmtiUtil.hpp"
#include "runtime/arguments.hpp"
#include "runtime/deoptimization.hpp"
#include "runtime/interfaceSupport.hpp"
#include "runtime/javaCalls.hpp"
#include "runtime/jfieldIDWorkaround.hpp"
#include "runtime/osThread.hpp"
#include "runtime/reflectionUtils.hpp"
#include "runtime/signature.hpp"
#include "runtime/thread.inline.hpp"
#include "runtime/vframe.hpp"
#include "runtime/vmThread.hpp"
#include "services/threadService.hpp"
#include "utilities/exceptions.hpp"
#include "utilities/preserveException.hpp"

static int get_jtid(jthread thread){
  JavaThread* current_thread = JavaThread::current();

  // if thread is NULL the current thread is used
  oop thread_oop;
  if (thread == NULL) {
    thread_oop = current_thread->threadObj();
  } else {
    thread_oop = JNIHandles::resolve_external_guard(thread);
  }
  if (thread_oop == NULL || !thread_oop->is_a(SystemDictionary::Thread_klass())){
    fprintf(stderr, "JVMTI_ERROR_INVALID_THREAD");
    return -1;
  }

  Handle thread_obj(current_thread, thread_oop);

  return java_lang_Thread::thread_id(thread_obj());
}
#else
static int get_jtid(jthread thread){
    return -999;
}
#endif


#endif


