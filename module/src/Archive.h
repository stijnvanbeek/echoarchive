#pragma once

#include <nap/resource.h>

namespace nap
{

	namespace echo
	{

		class NAPAPI Archive : public Resource
		{
			RTTI_ENABLE(Resource)
		public:
			std::string mDirectory; ///< Property: 'Directory' Location of the archive on disk

			// Inherited from Resource
			bool init(utility::ErrorState& errorState) override;


		private:
			std::vector<std::string> mFileNames; // Absolute filenames of all the archive files
		};

	}
}