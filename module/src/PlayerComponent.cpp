#include <PlayerComponent.h>

RTTI_BEGIN_CLASS(nap::echo::PlayerComponent)
		RTTI_PROPERTY("AudioComponent", &nap::echo::PlayerComponent::mAudioComponent, nap::rtti::EPropertyMetaData::Required)
		RTTI_PROPERTY("BufferPlayer", &nap::echo::PlayerComponent::mBufferPlayer, nap::rtti::EPropertyMetaData::Required)
		RTTI_PROPERTY("Archive", &nap::echo::PlayerComponent::mArchive, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::echo::PlayerComponentInstance)
		RTTI_CONSTRUCTOR(nap::EntityInstance &, nap::Component &)
RTTI_END_CLASS

namespace nap
{

	namespace echo
	{

		bool PlayerComponentInstance::init(utility::ErrorState &errorState)
		{
			auto resource = getComponent<PlayerComponent>();
			mPolyphonic = mAudioComponent->getObject<audio::PolyphonicInstance>();
			if (mPolyphonic == nullptr)
			{
				errorState.fail("PlayerComponentInstance: polyphonic not found");
				return false;
			}
			if (!mPolyphonic->getObjectMap(resource->mBufferPlayer->mID, mBufferPlayers, errorState))
				return false;

			mArchive = resource->mArchive;
			mNodeManager = &getEntityInstance()->getCore()->getService<audio::AudioService>()->getNodeManager();

			return true;
		}


		void PlayerComponentInstance::update(double deltaTime)
		{
			if (mSoundLayer == nullptr)
				mSoundLayer = std::make_unique<SoundLayer>(*this);

			if (mSoundLayer->isFinished())
				mSoundLayer = nullptr;
			else
				mSoundLayer->update(deltaTime);
		}


		SoundLayer::SoundLayer(nap::echo::PlayerComponentInstance &player) : mPlayer(player)
		{
			// Load audio file
			mFileLoaded = std::async(std::launch::async, [&](){
				mBuffer = mPlayer.mNodeManager->makeSafe<audio::MultiSampleBuffer>();
				mBuffer->resize(1, 0);
				mPlayer.mArchive->load(mBuffer->channels[0]);
			});

			// Fill the times and durations
			mTimes = { 0, 1, 2, 3, 4 };
			mDurations = { 4, 4, 4, 4, 4 };
		}


		void SoundLayer::update(double deltaTime)
		{
			// Wait until audio file has been loaded
			if (mFileLoaded.wait_for(std::chrono::microseconds(0)) != std::future_status::ready)
				return;

			// Check if audio file was loaded successfully
			auto &buffer = mBuffer->channels[0];
			if (buffer.empty())
			{
				mFinished = true;
				return;
			}

			mElapsedTime += deltaTime;
			if (mPosition < mTimes.size() && mElapsedTime > mTimes[mPosition])
			{
				Logger::info("Play");
				playSound();
				mPosition++;
			}

			// Only when all playing is done the SoundLayer including the buffer can be removed.
			if (mElapsedTime > mTimes[mTimes.size() - 1] + mDurations[mDurations.size() - 1])
				mFinished = true;
		}


		void SoundLayer::playSound()
		{
			auto voice = mPlayer.mPolyphonic->findFreeVoice();

			auto playerNode = mPlayer.mBufferPlayers[voice]->getChannel(0);
			playerNode->stop();
			playerNode->setBuffer(mBuffer.get());
			auto &buffer = mBuffer->channels[0];
			int bufferPos = math::random<float>(0.f, buffer.size() - 1);
			playerNode->play(0, bufferPos, 1.f);
			mPlayer.mPolyphonic->playOnChannels(voice, { math::random<unsigned int>(0, 1) }, mDurations[mPosition] * 1000.f);
		}

	}

}