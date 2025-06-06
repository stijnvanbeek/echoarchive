#pragma once

#include <Archive.h>

#include <audio/core/polyphonic.h>
#include <audio/object/bufferplayer.h>
#include <audio/object/filter.h>
#include <audio/object/delay.h>
#include <audio/component/audiocomponent.h>
#include <entity.h>

#include <future>

namespace nap
{

	namespace echo
	{

		// Forward declarations
		class PlayerComponentInstance;
		class SoundLayer;

		class PlayerComponent : public Component
		{
			RTTI_ENABLE(Component)
			DECLARE_COMPONENT(PlayerComponent, PlayerComponentInstance)
		public:
			ComponentPtr<audio::AudioComponent> mAudioComponent; ///< Property: 'AudioComponent'
			ResourcePtr<audio::Polyphonic> mPolyphonic = nullptr; ///< Property: 'Polyphonic'
			ResourcePtr<audio::BufferPlayer> mBufferPlayer = nullptr; ///< Property: 'BufferPlayer'
			ResourcePtr<audio::DelayObject> mDelay = nullptr; ///< Property: 'Delay'
			ResourcePtr<audio::Filter> mFilter = nullptr; ///< Property: 'Filter'
			ResourcePtr<Archive> mArchive = nullptr; ///< Property: 'Archive'
			float mModulationSpeed = 0.25f; ///< Property: 'ModulationSpeed'
			float mFilterBandWidth = 12.0f; ///< Property: 'FilterBandWidth'
			float mMinLayerSize = 5.0f; ///< Property: 'MinLayerSize'
			float mMaxLayerSize = 5.0f; ///< Property: 'MaxLayerSize'
			float mSoundDuration = 4.0f; ///< Property: 'SoundDuration'
			float mMinFilterPitch = 42.f;   ///< Property: 'MinimumFilterPitch'
			float mMaxFilterPitch = 100.f;  ///< Property: 'MaxFilterPitch'
			float mMinSoundTime = 0.5f;         ///< Property: 'MinSoundTime'
			float mMaxSoundTime = 1.5f;         ///< Property: 'MaxSoundTime'
			float mMinDelayTime = 0.2f;
			float mMaxDelayTime  = 2.f;
			float mMinWaitTime = 0.f;
			float mMaxWaitTime = 5.f;
			float mCompressorThreshold = -6.f;
			float mCompressorRatio = 4.f;
			float mCompressorMakeUp = 6.f;

		private:
		};


		class PlayerComponentInstance : public ComponentInstance
		{
			RTTI_ENABLE(ComponentInstance)
			friend class SoundLayer;
		public:
			PlayerComponentInstance(EntityInstance &entity, Component &resource) : ComponentInstance(entity, resource)
			{}

			bool init(utility::ErrorState &errorState) override;
			void update(double deltaTime) override;

		private:
			ComponentInstancePtr<audio::AudioComponent> mAudioComponent = { this, &PlayerComponent::mAudioComponent };
			audio::PolyphonicInstance* mPolyphonic = nullptr;
			audio::FilterInstance* mFilter = nullptr;
			audio::DelayObjectInstance* mDelay = nullptr;
			audio::PolyphonicInstance::ObjectMap<audio::BufferPlayerInstance> mBufferPlayers;
			ResourcePtr<Archive> mArchive = nullptr;
			audio::NodeManager* mNodeManager = nullptr;
			std::unique_ptr<SoundLayer> mSoundLayer = nullptr;
			PlayerComponent* mResource = nullptr;
			double mElapsedTime = 0.f;
			double mWaitTime = 0.f;
			double mElapsedWaitTime = 0.f;
		};


		class SoundLayer
		{
		public:
			SoundLayer(PlayerComponentInstance& player);
			void update(double deltaTime);
			bool isFinished() const { return mFinished; }

		private:
			void playSound();

			std::future<void> mFileLoaded;
			audio::SafeOwner<audio::MultiSampleBuffer> mBuffer = nullptr;
			PlayerComponentInstance& mPlayer;
			bool mFinished = false;
			double mElapsedTime = 0.f;
			std::vector<float> mTimes;
			std::vector<float> mDurations;
			int mPosition = 0;
		};

	}

}