/*
 * Copyright 2012, The Android Open Source Project
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

#ifndef WIFI_DISPLAY_SOURCE_H_

#define WIFI_DISPLAY_SOURCE_H_

#include "ANetworkSession.h"

#include <media/stagefright/foundation/AHandler.h>

#include <netinet/in.h>

namespace android {

struct IRemoteDisplayClient;
struct ParsedMessage;

// Represents the RTSP server acting as a wifi display source.
// Manages incoming connections, sets up Playback sessions as necessary.
struct WifiDisplaySource : public AHandler {
    static const unsigned kWifiDisplayDefaultPort = 7236;

    WifiDisplaySource(
            const sp<ANetworkSession> &netSession,
            const sp<IRemoteDisplayClient> &client);

    status_t start(const char *iface);
    status_t stop();

protected:
    virtual ~WifiDisplaySource();
    virtual void onMessageReceived(const sp<AMessage> &msg);

private:
    struct PlaybackSession;

    enum {
        kWhatStart,
        kWhatRTSPNotify,
        kWhatStop,
        kWhatReapDeadClients,
        kWhatPlaybackSessionNotify,
        kWhatKeepAlive,
    };

    struct ResponseID {
        int32_t mSessionID;
        int32_t mCSeq;

        bool operator<(const ResponseID &other) const {
            return mSessionID < other.mSessionID
                || (mSessionID == other.mSessionID
                        && mCSeq < other.mCSeq);
        }
    };

    typedef status_t (WifiDisplaySource::*HandleRTSPResponseFunc)(
            int32_t sessionID, const sp<ParsedMessage> &msg);

    static const int64_t kReaperIntervalUs = 1000000ll;

    static const int64_t kPlaybackSessionTimeoutSecs = 30;

    static const int64_t kPlaybackSessionTimeoutUs =
        kPlaybackSessionTimeoutSecs * 1000000ll;

    sp<ANetworkSession> mNetSession;
    sp<IRemoteDisplayClient> mClient;
    struct in_addr mInterfaceAddr;
    int32_t mSessionID;

    int32_t mClientSessionID;

    struct ClientInfo {
        AString mRemoteIP;
        AString mLocalIP;
        int32_t mLocalPort;
        int32_t mPlaybackSessionID;
        sp<PlaybackSession> mPlaybackSession;
    };
    ClientInfo mClientInfo;

    bool mReaperPending;

    int32_t mNextCSeq;

    KeyedVector<ResponseID, HandleRTSPResponseFunc> mResponseHandlers;

    status_t sendM1(int32_t sessionID);
    status_t sendM3(int32_t sessionID);
    status_t sendM4(int32_t sessionID);
    status_t sendM5(int32_t sessionID);
    status_t sendM16(int32_t sessionID);

    status_t onReceiveM1Response(
            int32_t sessionID, const sp<ParsedMessage> &msg);

    status_t onReceiveM3Response(
            int32_t sessionID, const sp<ParsedMessage> &msg);

    status_t onReceiveM4Response(
            int32_t sessionID, const sp<ParsedMessage> &msg);

    status_t onReceiveM5Response(
            int32_t sessionID, const sp<ParsedMessage> &msg);

    status_t onReceiveM16Response(
            int32_t sessionID, const sp<ParsedMessage> &msg);

    void registerResponseHandler(
            int32_t sessionID, int32_t cseq, HandleRTSPResponseFunc func);

    void onReceiveClientData(const sp<AMessage> &msg);

    void onDescribeRequest(
            int32_t sessionID,
            int32_t cseq,
            const sp<ParsedMessage> &data);

    void onOptionsRequest(
            int32_t sessionID,
            int32_t cseq,
            const sp<ParsedMessage> &data);

    void onSetupRequest(
            int32_t sessionID,
            int32_t cseq,
            const sp<ParsedMessage> &data);

    void onPlayRequest(
            int32_t sessionID,
            int32_t cseq,
            const sp<ParsedMessage> &data);

    void onPauseRequest(
            int32_t sessionID,
            int32_t cseq,
            const sp<ParsedMessage> &data);

    void onTeardownRequest(
            int32_t sessionID,
            int32_t cseq,
            const sp<ParsedMessage> &data);

    void onGetParameterRequest(
            int32_t sessionID,
            int32_t cseq,
            const sp<ParsedMessage> &data);

    void onSetParameterRequest(
            int32_t sessionID,
            int32_t cseq,
            const sp<ParsedMessage> &data);

    void sendErrorResponse(
            int32_t sessionID,
            const char *errorDetail,
            int32_t cseq);

    static void AppendCommonResponse(
            AString *response, int32_t cseq, int32_t playbackSessionID = -1ll);

    void scheduleReaper();
    void scheduleKeepAlive(int32_t sessionID);

    int32_t makeUniquePlaybackSessionID() const;

    sp<PlaybackSession> findPlaybackSession(
            const sp<ParsedMessage> &data, int32_t *playbackSessionID) const;

    // Disconnects the current client and shuts down its playback session
    // (if any). The reason for the disconnection is OK for orderly shutdown
    // or a nonzero error code.
    // A listener is notified accordingly.
    void disconnectClient(status_t err);

    DISALLOW_EVIL_CONSTRUCTORS(WifiDisplaySource);
};

}  // namespace android

#endif  // WIFI_DISPLAY_SOURCE_H_