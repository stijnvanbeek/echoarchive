#pragma once

#include <Archive.h>

#include <audio/resource/audiofileio.h>
#include <audio/node/inputnode.h>

#include <oscinputcomponent.h>
#include <component.h>
#include <entity.h>

#include <future>

namespace nap {

	namespace echo {

		class NAPAPI RecorderNode : public audio::Node
		{
		public:
			RecorderNode(audio::NodeManager& nodeManager) : audio::Node(nodeManager) { }
			void start(audio::SafePtr<audio::SampleBuffer> buffer);
			void stop();
			bool isRecording() const { return mIsRecording; }
			int getPosition() const { return mPosition.load(); }

			audio::InputPin input = { this };

		private:
			void process() override;

			audio::SafePtr<audio::SampleBuffer> mBuffer = nullptr;
			std::atomic<int> mPosition = 0;
			bool mIsRecording = false;
		};


		// Forward declarations
		class RecorderComponentInstance;


		class NAPAPI RecorderComponent : public Component
		{
			RTTI_ENABLE(Component)
			DECLARE_COMPONENT(RecorderComponent, RecorderComponentInstance)
		public:
			ResourcePtr<Archive> mArchive = nullptr; ///< Property: 'Archive' Pointer to the Archive resource
			float mMaxRecordingSize = 10.f; ///< Property: 'MaxRecordingSize' Maximum length of the recordings in seconds.
			int mInputChannel = 0; ///< Property: 'InputChannel'
			ComponentPtr<OSCInputComponent> mOSCInputComponent; ///< Property: 'OSCInputComponent' Receives messages with argument 0 or 1 to play or stop

		private:
		};


		class NAPAPI RecorderComponentInstance : public ComponentInstance
		{
			RTTI_ENABLE(ComponentInstance)
		public:
			RecorderComponentInstance(EntityInstance &entity, Component &resource) : ComponentInstance(entity, resource) {}

			// Inherited from Component
			bool init(utility::ErrorState &errorState) override;
			void onDestroy() override;

			void start();
			void stop();

		private:
			Slot<const OSCEvent&> mOSCMessageReceivedSlot = {this, &RecorderComponentInstance::oscMessageReceived };
			void oscMessageReceived(const OSCEvent& event);

			void normalizeRecording();

			audio::SafeOwner<audio::SampleBuffer> mBuffer; // Buffer to record into.
			audio::SafeOwner<audio::InputNode> mInputNode = nullptr;
			audio::SafeOwner<RecorderNode> mRecorderNode = nullptr;
			audio::NodeManager* mNodeManager = nullptr;
			ComponentInstancePtr<OSCInputComponent> mOSCInputComponent = { this, &RecorderComponent::mOSCInputComponent };
			RecorderComponent* mResource = nullptr;
		};

	}

}