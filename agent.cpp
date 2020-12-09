#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "jvmti.h"

void printGCTime(const char* type) {

  struct timeval tv;
  gettimeofday(&tv, NULL);

  struct tm localTime;
  localtime_r(&tv.tv_sec, &localTime);

  char *startTime = calloc(1, 128);

  strftime(startTime, (size_t) 128, "%a %b %d %Y %H:%M:%S", &localTime);

  fprintf(stderr, "GC %s: %s.%06d\n", type, startTime, (int)tv.tv_usec );
  fflush(stderr);

  if(startTime) free(startTime);

}

void JNICALL
garbageCollectionStart(jvmtiEnv *jvmti_env) {

  printGCTime("Start ");

}

void JNICALL
garbageCollectionFinish(jvmtiEnv *jvmti_env) {

  printGCTime("Finish");

}


JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM * jvm, char *options, void *reserved)
{
  jvmtiEnv *jvmti_env;

  jint returnCode = (*jvm)->GetEnv(jvm, (void **) &jvmti_env,
      JVMTI_VERSION_1_0);



  if (returnCode != JNI_OK)
    {
      fprintf(stderr,
          "The version of JVMTI requested (1.0) is not supported by this JVM.\n");
      return JVMTI_ERROR_UNSUPPORTED_VERSION;
    }


  jvmtiCapabilities *requiredCapabilities;

  requiredCapabilities = (jvmtiCapabilities*) calloc(1, sizeof(jvmtiCapabilities));
  if (!requiredCapabilities)
      {
        fprintf(stderr, "Unable to allocate memory\n");
        return JVMTI_ERROR_OUT_OF_MEMORY;
      }

  requiredCapabilities->can_generate_garbage_collection_events = 1;

  if (returnCode != JNI_OK)
    {
      fprintf(stderr, "C:\tJVM does not have the required capabilities (%d)\n",
          returnCode);
      exit(-1);
    }



  returnCode = (*jvmti_env)->AddCapabilities(jvmti_env, requiredCapabilities);


  jvmtiEventCallbacks *eventCallbacks;

  eventCallbacks = calloc(1, sizeof(jvmtiEventCallbacks));
  if (!eventCallbacks)
    {
      fprintf(stderr, "Unable to allocate memory\n");
      return JVMTI_ERROR_OUT_OF_MEMORY;
    }

  eventCallbacks->GarbageCollectionStart = &garbageCollectionStart;
  eventCallbacks->GarbageCollectionFinish = &garbageCollectionFinish;


  returnCode = (*jvmti_env)->SetEventCallbacks(jvmti_env,
      eventCallbacks, (jint) sizeof(*eventCallbacks));


  if (returnCode != JNI_OK)
    {
      fprintf(stderr, "C:\tError setting event callbacks (%d)\n",
          returnCode);
      exit(-1);
    }

  returnCode = (*jvmti_env)->SetEventNotificationMode(
      jvmti_env, JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START, (jthread) NULL);

  if (returnCode != JNI_OK)
    {
      fprintf(
          stderr,
          "C:\tJVM does not have the required capabilities, JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START (%d)\n",
          returnCode);
      exit(-1);
    }


  returnCode = (*jvmti_env)->SetEventNotificationMode(
      jvmti_env, JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, (jthread) NULL);

  if (returnCode != JNI_OK)
    {
      fprintf(
          stderr,
          "C:\tJVM does not have the required capabilities, JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH (%d)\n",
          returnCode);
      exit(-1);
    }


  if(requiredCapabilities) free(requiredCapabilities);
  if(eventCallbacks) free(eventCallbacks);

  return JVMTI_ERROR_NONE;
}




