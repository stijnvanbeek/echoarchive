#pragma once

#include <Archive.h>

#include <audio/core/polyphonic.h>
#include <audio/object/bufferplayer.h>
#include <audio/object/filter.h>
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
			ResourcePtr<audio::Filter> mFilter = nullptr; ///< Property: 'Filter'
			ResourcePtr<Archive> mArchive = nullptr; ///< Property: 'Archive'

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
			audio::PolyphonicInstance::ObjectMap<audio::BufferPlayerInstance> mBufferPlayers;
			ResourcePtr<Archive> mArchive = nullptr;
			audio::NodeManager* mNodeManager = nullptr;
			std::unique_ptr<SoundLayer> mSoundLayer = nullptr;
			PlayerComponent* mResource = nullptr;
			double mElapsedTime = 0.f;
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