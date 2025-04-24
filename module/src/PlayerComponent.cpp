#include <PlayerComponent.h>

#include <audio/core/graphobject.h>
#include <audio/utility/audiofunctions.h>

RTTI_BEGIN_CLASS(nap::echo::PlayerComponent)
    RTTI_PROPERTY("AudioComponent", &nap::echo::PlayerComponent::mAudioComponent, nap::rtti::EPropertyMetaData::Required)
    RTTI_PROPERTY("Polyphonic", &nap::echo::PlayerComponent::mPolyphonic, nap::rtti::EPropertyMetaData::Required)
    RTTI_PROPERTY("BufferPlayer", &nap::echo::PlayerComponent::mBufferPlayer, nap::rtti::EPropertyMetaData::Required)
    RTTI_PROPERTY("Filter", &nap::echo::PlayerComponent::mFilter, nap::rtti::EPropertyMetaData::Required)
    RTTI_PROPERTY("Archive", &nap::echo::PlayerComponent::mArchive, nap::rtti::EPropertyMetaData::Required)
    RTTI_PROPERTY("ModulationSpeed", &nap::echo::PlayerComponent::mModulationSpeed, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("FilterBandWidth", &nap::echo::PlayerComponent::mFilterBandWidth, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("MinLayerSize", &nap::echo::PlayerComponent::mMinLayerSize, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("MaxLayerSize", &nap::echo::PlayerComponent::mMaxLayerSize, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("SoundDuration", &nap::echo::PlayerComponent::mSoundDuration, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("MinFilterPitch", &nap::echo::PlayerComponent::mMinFilterPitch, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("MaxFilterPitch", &nap::echo::PlayerComponent::mMaxFilterPitch, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("MinSoundTime", &nap::echo::PlayerComponent::mMinSoundTime, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("MaxSoundTime", &nap::echo::PlayerComponent::mMaxSoundTime, nap::rtti::EPropertyMetaData::Default)
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
            mResource = getComponent<PlayerComponent>();
            auto graphObject = mAudioComponent->getObject<audio::GraphObjectInstance>();
            if (graphObject == nullptr)
            {
                errorState.fail("PlayerComponentInstance: graph object not found");
                return false;
            }

            mPolyphonic = graphObject->getObject<audio::PolyphonicInstance>(mResource->mPolyphonic->mID);
            if (mPolyphonic == nullptr)
            {
                errorState.fail("PlayerComponentInstance: polyphonic not found");
                return false;
            }

            mFilter = graphObject->getObject<audio::FilterInstance>(mResource->mFilter->mID);
            if (mFilter == nullptr)
            {
                errorState.fail("PlayerComponentInstance: filter not found");
                return false;
            }

            if (!mPolyphonic->getObjectMap(mResource->mBufferPlayer->mID, mBufferPlayers, errorState))
                return false;

            mArchive = mResource->mArchive;
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

            // Modulate filters
            mElapsedTime += deltaTime;
            float leftLFO = 0.5f * sin(mElapsedTime * math::PI * mResource->mModulationSpeed) + 0.5f;
            float rightLFO = 0.5f * sin(mElapsedTime * math::PI *  mResource->mModulationSpeed * 0.8f) + 0.5f;
            // Changed from hardcoded 42.f and 100.f to properties
            float leftPitch = math::lerp(mResource->mMinFilterPitch, mResource->mMaxFilterPitch, leftLFO);
            float rightPitch = math::lerp(mResource->mMinFilterPitch, mResource->mMaxFilterPitch, rightLFO);
            mFilter->getChannel(0)->setFrequency(audio::mtof(leftPitch));
            mFilter->getChannel(1)->setFrequency(audio::mtof(rightPitch));
            auto modDev = 0.5f * mResource->mFilterBandWidth;
            mFilter->getChannel(0)->setBand(audio::mtof(leftPitch + modDev) - audio::mtof(leftPitch - modDev));
            mFilter->getChannel(1)->setBand(audio::mtof(rightPitch + modDev) - audio::mtof(rightPitch - modDev));
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
            auto time = 0.f;
            for (auto i = 0; i < math::random(mPlayer.mResource->mMinLayerSize, mPlayer.mResource->mMaxLayerSize); ++i)
            {
                mTimes.emplace_back(time);
                time += math::random(mPlayer.mResource->mMinSoundTime, mPlayer.mResource->mMaxSoundTime);
                mDurations.emplace_back(mPlayer.mResource->mSoundDuration);
            }
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
                Logger::info("Play pos %i time %f", mPosition, mElapsedTime);
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