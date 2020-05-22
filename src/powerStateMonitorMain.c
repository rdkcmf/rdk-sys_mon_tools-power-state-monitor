/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>      /* Errors */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
#include <glib.h>
//#include <Msg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* C headers to be added here */

#ifdef __cplusplus
}
#endif

#include "pwrMgr.h"
#include "libIBus.h"
#include "iarmUtil.h"
#include "libIBus.h"
#include "libIBusDaemonInternal.h"
#include "libIBusDaemon.h"

static IARM_Bus_PWRMgr_PowerState_t gpowerState = IARM_BUS_PWRMGR_POWERSTATE_ON;

/* Function Declarations */
static void _lightsleepEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
typedef void* gpointer;

#define PADDING_SIZE 32
typedef enum _UIDev_PowerState_t {
    UIDEV_POWERSTATE_OFF,
    UIDEV_POWERSTATE_STANDBY,
    UIDEV_POWERSTATE_ON,
    UIDEV_POWERSTATE_UNKNOWN
} UIDev_PowerState_t;

typedef struct _UIMgr_Settings_t{
    unsigned int magic;
    unsigned int version;
    unsigned int length;
    UIDev_PowerState_t powerState;
    char padding[PADDING_SIZE];
} UIMgr_Settings_t ;

/*****************************************************************
 * Function Name: setModeSettings()
 * Input Arguments: int
 * Output Arguments: void
 * Description: This is to set the .standby/poweron flag based on
                the power MODE
*****************************************************************/
static void setModeSettings(int mode)
{
    int status=0;
    FILE *fptr = NULL;
    char setFlag[30] = {0};
    char deleteFlag[30] = {0};
    switch (mode){
       case 1:
           strcpy(setFlag, "/tmp/.power_on");
           strcpy(deleteFlag,"/tmp/.standby");
           printf("setModeSettings: Power ON Mode");
           break;
       case 0:
           strcpy(setFlag,"/tmp/.standby");
           strcpy(deleteFlag,"/tmp/.power_on");
           printf("setModeSettings: STANDBY Mode");
           system("sh /lib/rdk/lightsleep_handler.sh &");
           break;
       default:
           printf("setModeSettings: Unkown Argument..!");
    }
    fptr = fopen(setFlag, "w+");
    if(fptr == NULL){ //if file does not exist, create it
         printf("Failure: Not able to create a file (%s)\n",setFlag);
    }
    else{
         printf("Success: File (%s) created successfully..!\n",setFlag);
         fclose(fptr);
    }
    /* Deletes the standby/poweron file */
    status = remove(deleteFlag);
    if(status == 0){
         printf("Success: File (%s) deleted successfully\n",deleteFlag);
    }
}
/*****************************************************************
 * Function Name: _lightsleepEventHandler ()
 * Input Parameters: const char *, IARM_EventId_t, void *, size_t
 * Output Parameters: void
 * Description: Registers the power manager event call and 
                Detect the power transitions
*****************************************************************/
static void _lightsleepEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    printf("Lightsleep: Event handler invoked :: \n");
    if (strcmp(owner,IARM_BUS_PWRMGR_NAME) == 0)
    {
        switch (eventId)
        {
            case IARM_BUS_PWRMGR_EVENT_DEEPSLEEP_TIMEOUT:
            {
                printf("Time out event called \n");
                printf("Deep Sleep time out occured \n");
            }
              break;
            case IARM_BUS_PWRMGR_EVENT_MODECHANGED:
            {
                  IARM_Bus_PWRMgr_EventData_t *param = (IARM_Bus_PWRMgr_EventData_t *)data;
                        printf("Event IARM_BUS_PWRMGR_EVENT_MODECHANGED: State Changed %d to %d\n",
                                        param->data.state.curState, param->data.state.newState);
                         gpowerState = param->data.state.newState;
            }
              break;
            default:
              break;

        }
    }
    if(gpowerState == IARM_BUS_PWRMGR_POWERSTATE_ON)
    {
         setModeSettings (1);
    }
    else
    {
         setModeSettings (0);
    }
}
/*****************************************************************
 * Function Name: power_monitor_init
 * Input Parameters: (void)
 * Output Parameters: void
 * Description: Set the power flag during bootup and 
                Detect the power transitions
*****************************************************************/
int power_monitor_init(void)
{
    UIMgr_Settings_t uiSettings = {0};
    int ret = 0;
    const char *settingsFile = "/opt/uimgr_settings.bin";
    int fd = open(settingsFile, O_RDONLY);

    if (fd > 0) {
        lseek(fd, 0, SEEK_SET);
        ret = read(fd, &uiSettings, sizeof(uiSettings));
        if (ret == 0) {
            printf ("%s: power_monitor_init: Unable to read the file", __FILE__);
        }
        else if (ret > 0)
        {
            if (uiSettings.powerState == UIDEV_POWERSTATE_OFF)
                printf ("%s: power_monitor_init: OFF Mode",__FILE__);
            else if (uiSettings.powerState == UIDEV_POWERSTATE_STANDBY){
                printf ("%s: power_monitor_init: STANDBY Mode",__FILE__);
                setModeSettings(0);
            }
            else if (uiSettings.powerState == UIDEV_POWERSTATE_ON){
                printf ("%s: power_monitor_init: ON Mode",__FILE__);
                setModeSettings(1);
            }
        }
    }
    else
        printf ("%s: power_monitor_init: Unable to open the file /opt/uimgr_settings.bin");
    return 0;
}
/*********************************************************
 * Function Name: main ()
 * Input Parameters: int argc, char *argv[]
 * Output Parameters: int
 * Description: Registers the lightsleep thread functions
*********************************************************/
int main(int argc, char *argv[])
{
    GAsyncQueue *msgQueuePtr=NULL;
    GAsyncQueue *gQpointer=NULL;
    IARM_Bus_PWRMgr_GetPowerState_Param_t param;

    power_monitor_init();
    /* IARM BUS Init call with lightsleep module name */
    printf("Initializing the lightsleep power notifier..!\n");
    if(IARM_RESULT_SUCCESS != IARM_Bus_Init("IARM_BUS_LIGHTSLEEP_MODULE"))
    {
        printf("%s : Failed in IARM Bus IARM_Bus_Init..!\n",__FUNCTION__,__LINE__);
        return 0; 
    }
    /* IARM Bus Connect Call..! */
    else
    {
        printf("Success: IARM_Bus_Init..!\n");
    }
    if (IARM_RESULT_SUCCESS != IARM_Bus_Connect())
    {
        printf("%s : Failed in  IARM_Bus_Connect..!\n",__FUNCTION__);
        //pthread_exit(0); /* exit */
        //return NULL;
        return 0;
    }
    else{
       printf("Success: IARM_Bus_Connect..!\n");
    }
    /* Register the event callback function..! */
    IARM_Bus_RegisterEvent(IARM_BUS_PWRMGR_EVENT_MAX);
    IARM_Bus_RegisterEventHandler(IARM_BUS_PWRMGR_NAME,  IARM_BUS_PWRMGR_EVENT_MODECHANGED, _lightsleepEventHandler);
    printf("%s : Registering Callback for Power Mode Change Notification..!\n",__FUNCTION__);
    /* This call will make the process alive */
    while (true)
    {
       msgQueuePtr = g_async_queue_new ();
       if ( msgQueuePtr == NULL ){
            printf("Not able to create a queue..!");
       }
       else{
            printf("Wait Call for message pop..!\n");
       }
       g_async_queue_pop (msgQueuePtr);
    }
    exit(0); /* exit */  
}

