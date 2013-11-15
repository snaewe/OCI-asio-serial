#include <boost/test/auto_unit_test.hpp>
#include "../GPSLib/Util.h"

BOOST_AUTO_TEST_CASE(LexicalCastDefaultEmptyTest) 
{
	BOOST_REQUIRE_EQUAL(3, GPSLib::lexical_cast_default<int>("", 3));
}

BOOST_AUTO_TEST_CASE(LexicalCastDefaultBadTest) 
{
	BOOST_REQUIRE_EQUAL(3, GPSLib::lexical_cast_default<int>("Fred", 3));
}

BOOST_AUTO_TEST_CASE(LexicalCastDefaultTest) 
{
	BOOST_REQUIRE_EQUAL(9, GPSLib::lexical_cast_default<int>("9", 3));
}

BOOST_AUTO_TEST_CASE(HexConvert) 
{
	BOOST_REQUIRE_EQUAL(240, boost::lexical_cast<GPSLib::byte_from_hex>("F0"));
}


#include <boost/regex.hpp>
#include <boost/date_time.hpp>

const boost::regex hms("(\\d{2})(\\d{2})(\\d{2})(?:(.\\d*))?");
boost::posix_time::time_duration DecodeTime(std::vector<std::string>::iterator &i) {
	boost::smatch m;
	if (boost::regex_match(*i, m, hms)) {
		i++;  // consume the match
		boost::posix_time::hours hr(boost::lexical_cast<int>(m[1].str()));
		boost::posix_time::minutes min(boost::lexical_cast<int>(m[2].str()));
		boost::posix_time::seconds sec(boost::lexical_cast<int>(m[3].str()));
		boost::posix_time::milliseconds ms(boost::lexical_cast<int>(boost::lexical_cast<double>(m[4].str())*1000));
		return boost::posix_time::time_duration(hr+min+sec+ms);
	}
	else
		throw std::invalid_argument(*i + " is not hms");
}

BOOST_AUTO_TEST_CASE(MillisecondTest) 
{
	std::vector<std::string> v;
	v.push_back("010203.4");
	std::vector<std::string>::iterator i = v.begin();
	BOOST_REQUIRE_EQUAL(boost::posix_time::duration_from_string("1:2:3.4"), DecodeTime(i));
}

