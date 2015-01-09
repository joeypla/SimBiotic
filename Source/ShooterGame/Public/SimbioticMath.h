#ifndef SIMBIOTIC_MATH_H
#define SIMBIOTIC_MATH_H


namespace SimBioticMath
{
	struct float3
	{
		float x;
		float y;
		float z;
	};

	float3 GetClosestUnitVector(float3& vector);
};
#endif