#include <boost/test/auto_unit_test.hpp>
#include "../GPSLib/Util.h"

BOOST_AUTO_TEST_CASE(DMToDecimalDegreeTest) 
{
	BOOST_REQUIRE_CLOSE(-79.948862, GPSLib::ToDecimalDegree(79, 56.93172, "W"), 0.01);
}

BOOST_AUTO_TEST_CASE(DMSToDecimalDegreeTest) 
{
	BOOST_REQUIRE_CLOSE(-87.728056, GPSLib::ToDecimalDegree(87, 43, 41, "W"), 0.01);
}

BOOST_AUTO_TEST_CASE(LongToDegreesMinutesSecondsTest) 
{
	int expectedDegrees=87, expectedMinutes=43, expectedSeconds=41;
	std::string expectedHemisphere = "W";
	double decimalDegrees = GPSLib::ToDecimalDegree(
		expectedDegrees, expectedMinutes, expectedSeconds, expectedHemisphere);
	int actualDegrees, actualMinutes, actualSeconds;
	std::string actualHemisphere;
	GPSLib::LonToDegreesMinutesSeconds(decimalDegrees, actualDegrees, actualMinutes, actualSeconds, actualHemisphere);
	BOOST_REQUIRE_EQUAL(expectedDegrees, actualDegrees);
	BOOST_REQUIRE_EQUAL(expectedMinutes, actualMinutes);
	BOOST_REQUIRE_EQUAL(expectedSeconds, actualSeconds);
	BOOST_REQUIRE_EQUAL(expectedHemisphere, actualHemisphere);
}

BOOST_AUTO_TEST_CASE(LatToDegreesMinutesSecondsTest) 
{
	int expectedDegrees=87, expectedMinutes=43, expectedSeconds=41;
	std::string expectedHemisphere = "S";
	double decimalDegrees = GPSLib::ToDecimalDegree(
		expectedDegrees, expectedMinutes, expectedSeconds, expectedHemisphere);
	int actualDegrees, actualMinutes, actualSeconds;
	std::string actualHemisphere;
	GPSLib::LatToDegreesMinutesSeconds(decimalDegrees, actualDegrees, actualMinutes, actualSeconds, actualHemisphere);
	BOOST_REQUIRE_EQUAL(expectedDegrees, actualDegrees);
	BOOST_REQUIRE_EQUAL(expectedMinutes, actualMinutes);
	BOOST_REQUIRE_EQUAL(expectedSeconds, actualSeconds);
	BOOST_REQUIRE_EQUAL(expectedHemisphere, actualHemisphere);
}
