#pragma once

#include <audio/resource/audiofileio.h>
#include <audio/utility/audiotypes.h>
#include <nap/resource.h>
#include <utility/threading.h>
#include <random>

namespace nap
{

	namespace echo
	{

		void normalize(audio::SampleBuffer& buffer, int length);


		class NAPAPI Archive : public Resource
		{
			RTTI_ENABLE(Resource)
		public:
			std::string mDirectory; ///< Property: 'Directory' Location of the archive on disk

			// Inherited from Resource
			bool init(utility::ErrorState& errorState) override;

			void save(audio::SampleBuffer& buffer, int length, float sampleRate);
			void load(audio::SampleBuffer& buffer);

		private:
			std::vector<std::string> mFileNames; // Absolute filenames of all the archive files
			moodycamel::ConcurrentQueue<std::string> mJustRecordedQueue;
			int mPosition = 0; // Current position in the mFileNames list.

			std::mt19937 mRandomGenerator;
		};

	}
}