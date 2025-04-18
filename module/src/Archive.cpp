#include "Archive.h"

#include <nap/datetime.h>
#include <utility/fileutils.h>
#include <mathutils.h>

#include <nap/logger.h>

RTTI_BEGIN_CLASS(nap::echo::Archive)
		RTTI_PROPERTY("Directory", &nap::echo::Archive::mDirectory, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

namespace nap
{
	namespace echo
	{

		void normalize(audio::SampleBuffer& buffer, int length)
		{
			// Determine maximum
			float max = 0.f;
			for (auto i = 0; i < length; ++i)
			{
				auto& value = buffer[i];
				if (value > max)
					max = value;
				if (value < -max)
					max = -value;
			}

			// Normalize
			float multiplier = 1.f / max;
			for (auto i = 0; i < length; ++i)
				buffer[i] *= multiplier;
		}


		bool Archive::init(utility::ErrorState &errorState)
		{
			utility::listDir(mDirectory.c_str(), mFileNames, false);
			return true;
		}


		void Archive::save(audio::SampleBuffer &buffer, int length, float sampleRate)
		{
			normalize(buffer, length);
			DateTime dateTime(getCurrentTime());
			auto fileName = dateTime.toString() + ".wav";
			auto path = mDirectory + "/" + fileName;
			{
				audio::AudioFileDescriptor audioFile(path, audio::AudioFileDescriptor::Mode::WRITE, 1, sampleRate);
				assert(audioFile.isValid());
				audioFile.write(buffer.data(), length);
			}
			mFileNames.emplace_back(fileName);
			mPlaybackQueue.enqueue(fileName);
		}


		void Archive::load(audio::SampleBuffer &buffer)
		{
			std::string fileName;
			if (!mPlaybackQueue.try_dequeue(fileName))
				fileName = mFileNames[math::random<int>(0, mFileNames.size() - 1)];
			auto path = mDirectory + "/" + fileName;
			audio::AudioFileDescriptor audioFile(path, audio::AudioFileDescriptor::Mode::READ);

			buffer.clear();
			if (!audioFile.isValid())
			{
				Logger::error("Failed to load audio file: %s", fileName.c_str());
				return;
			}
			bool eof = false;
			int blockSize = 512;
			while (!eof)
			{
				int pos = buffer.size();
				buffer.resize(pos + blockSize, 0.f);
				int framesRead = audioFile.read(&buffer[pos], blockSize);
				if (framesRead < blockSize)
				{
					eof = true;
					buffer.resize(pos + framesRead);
				}
			}
			Logger::info("Loaded audio file: %s", fileName.c_str());
		}

	}
}