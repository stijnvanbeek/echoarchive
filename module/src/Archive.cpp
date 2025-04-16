#include "Archive.h"

RTTI_BEGIN_CLASS(nap::echo::Archive)
		RTTI_PROPERTY("Directory", &nap::echo::Archive::mDirectory, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

namespace nap
{
	namespace echo
	{

		bool Archive::init(utility::ErrorState &errorState)
		{
			return true;
		}

	}
}