// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2024 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "FrameInterpolationSwapchainVK_Helpers.h"

#ifdef _WIN32
#include <dwmapi.h>
#endif  // #ifdef _WIN32

void waitForPerformanceCount(const int64_t targetCount)
{
    int64_t currentCount = 0;
    do
    {
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentCount));
    } while (currentCount < targetCount);
}

VkResult VulkanQueue::submit(VkCommandBuffer commandBuffer, SubmissionSemaphores& semaphoresToWait, SubmissionSemaphores& semaphoresToSignal, VkFence fence)
{
    VkSubmitInfo submitInfo         = {};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = semaphoresToWait.count;
    submitInfo.pWaitSemaphores      = semaphoresToWait.semaphores;
    submitInfo.pWaitDstStageMask    = semaphoresToWait.waitStages;
    submitInfo.signalSemaphoreCount = semaphoresToSignal.count;
    submitInfo.pSignalSemaphores    = semaphoresToSignal.semaphores;

    if (commandBuffer == VK_NULL_HANDLE)
    {
        submitInfo.commandBufferCount = 0;
        submitInfo.pCommandBuffers    = nullptr;
    }
    else
    {
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &commandBuffer;
    }

    VkTimelineSemaphoreSubmitInfo timelineSemaphoreSubmitInfo = {};
    if (semaphoresToSignal.count == 0 && semaphoresToWait.count == 0)
    {
        submitInfo.pNext = nullptr;
    }
    else
    {
        timelineSemaphoreSubmitInfo.sType                     = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timelineSemaphoreSubmitInfo.pNext                     = nullptr;
        timelineSemaphoreSubmitInfo.waitSemaphoreValueCount   = semaphoresToWait.count;
        timelineSemaphoreSubmitInfo.pWaitSemaphoreValues      = semaphoresToWait.values;
        timelineSemaphoreSubmitInfo.signalSemaphoreValueCount = semaphoresToSignal.count;
        timelineSemaphoreSubmitInfo.pSignalSemaphoreValues    = semaphoresToSignal.values;

        submitInfo.pNext = &timelineSemaphoreSubmitInfo;
    }

    VkResult res = VK_SUCCESS;
    if (submitFunc != nullptr)
        res = submitFunc(1, &submitInfo, fence);
    else
        res = vkQueueSubmit(queue, 1, &submitInfo, fence);

    semaphoresToWait.reset();
    semaphoresToSignal.reset();

    return res;
}

VkResult VulkanQueue::submit(VkCommandBuffer commandBuffer, VkSemaphore timelineSemaphore, uint64_t signalValue)
{
    SubmissionSemaphores semaphoresToWait;
    SubmissionSemaphores semaphoresToSignal;

    semaphoresToSignal.add(timelineSemaphore, signalValue);

    return submit(commandBuffer, semaphoresToWait, semaphoresToSignal);
}

VkResult CreateShaderModule(VkDevice device, size_t codeSize, const uint32_t* pCode, VkShaderModule* pModule, const VkAllocationCallbacks* pAllocator)
{
    VkShaderModuleCreateInfo info = {};
    info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.pNext                    = nullptr;
    info.flags                    = 0;
    info.codeSize                 = codeSize;
    info.pCode                    = pCode;

    return vkCreateShaderModule(device, &info, pAllocator, pModule);
}
#ifndef _WIN32
// Implement QueryPerformanceCounter
BOOL QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return FALSE;
    }
    lpPerformanceCount->QuadPart = (uint64_t)now.tv_sec * 1000000000LL + now.tv_nsec;
    return TRUE;
}

// 实现QueryPerformanceFrequency
BOOL QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency) {
    lpFrequency->QuadPart = 1000000000LL; // 纳秒精度
    return TRUE;
}



static void* thread_wrapper(void* arg) {
    ThreadParams* params = static_cast<ThreadParams*>(arg);
    unsigned long result = params->lpStartAddress(params->lpParameter);
    delete params;
    return reinterpret_cast<void*>(result);
}

// Implement CreateThread
HANDLE CreateThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    SIZE_T dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    
    if (dwStackSize > 0) {
        pthread_attr_setstacksize(&attr, dwStackSize);
    }
    
    pthread_t thread;
    ThreadParams* params = new ThreadParams{lpStartAddress, lpParameter};
    
    int result = pthread_create(&thread, &attr, thread_wrapper, params);
    pthread_attr_destroy(&attr);
    
    if (result != 0) {
        delete params;
        return NULL;
    }
    
    if (lpThreadId) {
        *lpThreadId = static_cast<DWORD>(reinterpret_cast<uintptr_t>(thread));
    }
    
    if (dwCreationFlags & CREATE_SUSPENDED) {
        pthread_kill(thread, SIGSTOP);
    }
    
    return reinterpret_cast<HANDLE>(thread);
}

// Implement SetThreadPriority
BOOL SetThreadPriority(HANDLE hThread, int nPriority) {
    pthread_t thread = reinterpret_cast<pthread_t>(hThread);
    
    // 映射Windows优先级到Linux优先级
    int policy;
    struct sched_param param;
    pthread_getschedparam(thread, &policy, &param);
    
    const int min_prio = sched_get_priority_min(policy);
    const int max_prio = sched_get_priority_max(policy);
    const int range = max_prio - min_prio;
    
    // Windows优先级: THREAD_PRIORITY_IDLE (-15) 到 THREAD_PRIORITY_TIME_CRITICAL (15)
    int linux_prio = min_prio + (nPriority + 15) * range / 30;
    linux_prio = std::clamp(linux_prio, min_prio, max_prio);
    
    param.sched_priority = linux_prio;
    return pthread_setschedparam(thread, policy, &param) == 0;
}

// 实现SetThreadDescription
BOOL SetThreadDescription(HANDLE hThread, PCWSTR lpThreadDescription) {
    pthread_t thread = reinterpret_cast<pthread_t>(hThread);
    
    char name[16] = {0}; // Linux线程名最大长度
    wcstombs(name, lpThreadDescription, sizeof(name)-1);
    
    return pthread_setname_np(thread, name) == 0;
}

// 实现CloseHandle
BOOL CloseHandle(HANDLE hObject) {
    if (!hObject) return FALSE;
    // 对于线程句柄，需要detach
    if (pthread_kill(reinterpret_cast<pthread_t>(hObject), 0) == 0) {
        pthread_detach(reinterpret_cast<pthread_t>(hObject));
    }
    return TRUE;
}



// 实现CreateEvent
HANDLE CreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName) {
    LinuxEvent* event = new LinuxEvent();
    pthread_mutex_init(&event->mutex, NULL);
    pthread_cond_init(&event->cond, NULL);
    event->signaled = bInitialState;
    return static_cast<HANDLE>(event);
}

// 实现SetEvent
BOOL SetEvent(HANDLE hEvent) {
    LinuxEvent* event = static_cast<LinuxEvent*>(hEvent);
    pthread_mutex_lock(&event->mutex);
    event->signaled = true;
    pthread_cond_broadcast(&event->cond);
    pthread_mutex_unlock(&event->mutex);
    return TRUE;
}

// 实现ResetEvent
BOOL ResetEvent(HANDLE hEvent) {
    LinuxEvent* event = static_cast<LinuxEvent*>(hEvent);
    pthread_mutex_lock(&event->mutex);
    event->signaled = false;
    pthread_mutex_unlock(&event->mutex);
    return TRUE;
}

// 实现WaitForSingleObject
DWORD WaitForSingleObject(HANDLE hEvent, DWORD dwMilliseconds) {
    LinuxEvent* event = static_cast<LinuxEvent*>(hEvent);
    pthread_mutex_lock(&event->mutex);
    
    if (!event->signaled) {
        if (dwMilliseconds == INFINITE) {
            pthread_cond_wait(&event->cond, &event->mutex);
        } else {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec += (dwMilliseconds % 1000) * 1000000;
            ts.tv_sec += dwMilliseconds / 1000 + ts.tv_nsec / 1000000000;
            ts.tv_nsec %= 1000000000;
            
            int result = pthread_cond_timedwait(&event->cond, &event->mutex, &ts);
            if (result == ETIMEDOUT) {
                pthread_mutex_unlock(&event->mutex);
                return WAIT_TIMEOUT;
            }
        }
    }
    
    // 对于自动重置事件，重置状态
    event->signaled = false;
    pthread_mutex_unlock(&event->mutex);
    return WAIT_OBJECT_0;
}

// 实现SafeCloseHandle
void SafeCloseHandle(HANDLE phObject) {
    if (!phObject || !phObject) return;
    
    // 先尝试作为事件句柄处理
    LinuxEvent* event = static_cast<LinuxEvent*>(phObject);
    if (event) {
        pthread_cond_destroy(&event->cond);
        pthread_mutex_destroy(&event->mutex);
        delete event;
    } else {
        // 作为线程句柄处理
        pthread_t thread = reinterpret_cast<pthread_t>(phObject);
        pthread_detach(thread);
    }
    
    phObject = NULL;
}
#endif