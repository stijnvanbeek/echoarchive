#include "RecorderComponent.h"

#include <audio/service/audioservice.h>
#include <nap/core.h>
#include <nap/logger.h>

RTTI_BEGIN_CLASS(nap::echo::RecorderComponent)
		RTTI_PROPERTY("Archive", &nap::echo::RecorderComponent::mArchive, nap::rtti::EPropertyMetaData::Required)
		RTTI_PROPERTY("OSCInputComponent", &nap::echo::RecorderComponent::mOSCInputComponent, nap::rtti::EPropertyMetaData::Required)
		RTTI_PROPERTY("MaxRecordingSize", &nap::echo::RecorderComponent::mMaxRecordingSize, nap::rtti::EPropertyMetaData::Default)
		RTTI_PROPERTY("InputChannel", &nap::echo::RecorderComponent::mInputChannel, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::echo::RecorderComponentInstance)
		RTTI_CONSTRUCTOR(nap::EntityInstance &, nap::Component &)
RTTI_END_CLASS

namespace nap
{

	namespace echo
	{

		void RecorderNode::start(audio::SafePtr<audio::SampleBuffer> buffer)
		{
			mPosition = 0;
			mBuffer = buffer;
			mIsRecording = true;
		}


		void RecorderNode::stop()
		{
			mBuffer = nullptr;
			mIsRecording = false;
		}


		void RecorderNode::process()
		{
			if (mBuffer == nullptr)
			{
				return;
			}

			auto buffer = mBuffer;
			auto inputBuffer = input.pull();
			if (inputBuffer == nullptr)
				return;

			for (auto i = 0; i < inputBuffer->size(); ++i)
			{
				(*buffer)[mPosition++] = (*inputBuffer)[i];
				if (mPosition >= buffer->size())
				{
					mBuffer = nullptr;
					return;
				}
			}
		}


		bool RecorderComponentInstance::init(utility::ErrorState &errorState)
		{
			mResource = getComponent<RecorderComponent>();
			mNodeManager = &getEntityInstance()->getCore()->getService<audio::AudioService>()->getNodeManager();

			mInputNode = mNodeManager->makeSafe<audio::InputNode>(*mNodeManager);
			mInputNode->setInputChannel(mResource->mInputChannel);
			mRecorderNode = mNodeManager->makeSafe<RecorderNode>(*mNodeManager);
			mRecorderNode->input.connect(mInputNode->audioOutput);

			mBuffer = mNodeManager->makeSafe<audio::SampleBuffer>();
			mBuffer->resize(mResource->mMaxRecordingSize * mNodeManager->getSampleRate());

			mNodeManager->registerRootProcess(*mRecorderNode);
			mOSCInputComponent->messageReceived.connect(mOSCMessageReceivedSlot);

			return true;
		}


		void RecorderComponentInstance::onDestroy()
		{
			mNodeManager->unregisterRootProcess(*mRecorderNode);
		}


		void RecorderComponentInstance::start()
		{
			if (!mRecorderNode->isRecording())
			{
				mRecorderNode->start(mBuffer.get());
				Logger::debug("Recording starting");
			}
		}


		void RecorderComponentInstance::stop()
		{
			if (mRecorderNode->isRecording())
			{
				mRecorderNode->stop();
				Logger::debug("Recording finished");
				mResource->mArchive->save(*mBuffer, mRecorderNode->getPosition(), mNodeManager->getSampleRate());
			}
		}


		void RecorderComponentInstance::oscMessageReceived(const nap::OSCEvent &event)
		{
			auto argument = event.getArgument(0);
			if (!argument->isInt()) {
				Logger::warn("RecorderComponentInstance: received non integer OSC message argument");
				return;
			}
			int value = argument->asInt();
			if (value == 1)
				start();
			else if (value == 0)
				stop();
			else
				Logger::warn("RecorderComponentInstance: invalid integer OSC message: %i", value);
		}


	}

}