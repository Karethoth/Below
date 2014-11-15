#define GLM_FORCE_RADIANS
#include "smooth.hh"


namespace smoothHelpers
{
	AngleAxis operator* ( const AngleAxis& lhs, float rhs )
	{
		AngleAxis result{ lhs.angle, lhs.axis };
		result.angle *= rhs;
		return result;
	}



	AngleAxis operator/ ( const AngleAxis& lhs, float rhs )
	{
		AngleAxis result{ lhs.angle, lhs.axis };
		result.angle /= rhs;
		return result;
	}
}

