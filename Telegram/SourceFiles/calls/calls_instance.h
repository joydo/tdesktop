/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "mtproto/sender.h"
#include "calls/calls_call.h"
#include "calls/calls_group_call.h"
#include "calls/calls_choose_join_as.h"

namespace Platform {
enum class PermissionType;
} // namespace Platform

namespace Media::Audio {
class Track;
} // namespace Media::Audio

namespace Main {
class Session;
} // namespace Main

namespace Calls::Group {
struct JoinInfo;
class Panel;
} // namespace Calls::Group

namespace Calls {

class Panel;

class Instance
	: private Call::Delegate
	, private GroupCall::Delegate
	, private base::Subscriber
	, public base::has_weak_ptr {
public:
	Instance();
	~Instance();

	void startOutgoingCall(not_null<UserData*> user, bool video);
	void startOrJoinGroupCall(
		not_null<PeerData*> peer,
		const QString &joinHash = QString(),
		bool confirmNeeded = false);
	void handleUpdate(
		not_null<Main::Session*> session,
		const MTPUpdate &update);

	// Called by Data::GroupCall when it is appropriate by the 'version'.
	void applyGroupCallUpdateChecked(
		not_null<Main::Session*> session,
		const MTPUpdate &update);

	void showInfoPanel(not_null<Call*> call);
	void showInfoPanel(not_null<GroupCall*> call);
	[[nodiscard]] Call *currentCall() const;
	[[nodiscard]] rpl::producer<Call*> currentCallValue() const;
	[[nodiscard]] GroupCall *currentGroupCall() const;
	[[nodiscard]] rpl::producer<GroupCall*> currentGroupCallValue() const;
	[[nodiscard]] bool inCall() const;
	[[nodiscard]] bool inGroupCall() const;
	[[nodiscard]] bool hasActivePanel(
		not_null<Main::Session*> session) const;
	bool activateCurrentCall(const QString &joinHash = QString());
	bool minimizeCurrentActiveCall();
	bool closeCurrentActiveCall();
	auto getVideoCapture()
		-> std::shared_ptr<tgcalls::VideoCaptureInterface> override;
	void requestPermissionsOrFail(Fn<void()> onSuccess, bool video = true);

	void setCurrentAudioDevice(bool input, const QString &deviceId);

	[[nodiscard]] bool isQuitPrevent();

private:
	using CallSound = Call::Delegate::CallSound;
	using GroupCallSound = GroupCall::Delegate::GroupCallSound;

	[[nodiscard]] not_null<Call::Delegate*> getCallDelegate() {
		return static_cast<Call::Delegate*>(this);
	}
	[[nodiscard]] not_null<GroupCall::Delegate*> getGroupCallDelegate() {
		return static_cast<GroupCall::Delegate*>(this);
	}
	[[nodiscard]] DhConfig getDhConfig() const override {
		return _dhConfig;
	}

	not_null<Media::Audio::Track*> ensureSoundLoaded(const QString &key);
	void playSoundOnce(const QString &key);

	void callFinished(not_null<Call*> call) override;
	void callFailed(not_null<Call*> call) override;
	void callRedial(not_null<Call*> call) override;
	void callRequestPermissionsOrFail(
			Fn<void()> onSuccess,
			bool video) override {
		requestPermissionsOrFail(std::move(onSuccess), video);
	}
	void callPlaySound(CallSound sound) override;

	void groupCallFinished(not_null<GroupCall*> call) override;
	void groupCallFailed(not_null<GroupCall*> call) override;
	void groupCallRequestPermissionsOrFail(Fn<void()> onSuccess) override {
		requestPermissionsOrFail(std::move(onSuccess), false);
	}
	void groupCallPlaySound(GroupCallSound sound) override;

	void createCall(not_null<UserData*> user, Call::Type type, bool video);
	void destroyCall(not_null<Call*> call);

	void createGroupCall(
		Group::JoinInfo info,
		const MTPInputGroupCall &inputCall);
	void destroyGroupCall(not_null<GroupCall*> call);

	void requestPermissionOrFail(
		Platform::PermissionType type,
		Fn<void()> onSuccess);

	void refreshDhConfig();
	void refreshServerConfig(not_null<Main::Session*> session);
	bytes::const_span updateDhConfig(const MTPmessages_DhConfig &data);

	void destroyCurrentCall();
	void handleCallUpdate(
		not_null<Main::Session*> session,
		const MTPPhoneCall &call);
	void handleSignalingData(
		not_null<Main::Session*> session,
		const MTPDupdatePhoneCallSignalingData &data);
	void handleGroupCallUpdate(
		not_null<Main::Session*> session,
		const MTPUpdate &update);

	DhConfig _dhConfig;

	crl::time _lastServerConfigUpdateTime = 0;
	base::weak_ptr<Main::Session> _serverConfigRequestSession;
	std::weak_ptr<tgcalls::VideoCaptureInterface> _videoCapture;

	std::unique_ptr<Call> _currentCall;
	rpl::event_stream<Call*> _currentCallChanges;
	std::unique_ptr<Panel> _currentCallPanel;

	std::unique_ptr<GroupCall> _currentGroupCall;
	rpl::event_stream<GroupCall*> _currentGroupCallChanges;
	std::unique_ptr<Group::Panel> _currentGroupCallPanel;

	base::flat_map<QString, std::unique_ptr<Media::Audio::Track>> _tracks;

	Group::ChooseJoinAsProcess _chooseJoinAs;

};

} // namespace Calls
