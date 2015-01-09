#include "ShooterGame.h"

#include "SimBioticMath.h"

#include <math.h>


namespace SimBioticMath
{
	float3 GetClosestUnitVector(float3& vector)
	{
		float3 returnVector;
		returnVector.x = 0.0f;
		returnVector.y = 0.0f;
		returnVector.z = 0.0f;

		if (abs(vector.x) >= abs(vector.y) && abs(vector.x) >= abs(vector.z))
		{
			if (vector.x > 0.0f)
			{
				returnVector.x = 1.0f;
				return returnVector;
			}
			else
			{
				returnVector.x = -1.0f;
				return returnVector;
			}
		}

		if (abs(vector.y) >= abs(vector.x) && abs(vector.y) >= abs(vector.z))
		{
			if (vector.y > 0.0f)
			{
				returnVector.y = 1.0f;
				return returnVector;
			}
			else
			{
				returnVector.y = -1.0f;
				return returnVector;
			}
		}

		if (abs(vector.z) >= abs(vector.x) && abs(vector.z) >= abs(vector.y))
		{
			if (vector.z > 0.0f)
			{
				returnVector.z = 1.0f;
				return returnVector;
			}
			else
			{
				returnVector.z = -1.0f;
				return returnVector;
			}
		}
		return returnVector;
	}
}