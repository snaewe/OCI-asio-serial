#include <boost/lexical_cast.hpp>

namespace GPSLib {
	// http://stackoverflow.com/questions/1070497/c-convert-hex-string-to-signed-integer
	class byte_from_hex {
		unsigned int value;
	public:
		operator unsigned char() const { return value & 0xFF; }
		friend std::istream& operator>>(std::istream& in, byte_from_hex& out) {
			in >> std::hex >> out.value;
			return in;
		}
	};

	// because fields to convert may be empty
    template<typename Target>
    inline Target lexical_cast_default(const std::string &arg, const Target &def) {
		try {
			return boost::lexical_cast<Target>(arg);
		} catch (boost::bad_lexical_cast &) {
			return def;
		}
	}


	// http://en.wikipedia.org/wiki/Geographic_coordinate_conversion
	inline double ToDecimalDegree(double degrees, double minutes, double seconds, const std::string &hemisphere) {
		degrees+= minutes/60.0 + seconds/3600.0;
		if ((hemisphere == "S") || (hemisphere == "W")) degrees = -degrees;
		return degrees;
	}

	inline double ToDecimalDegree(double degrees, double minutes, const std::string &hemisphere) {
		return ToDecimalDegree(degrees, minutes, 0.0, hemisphere);
	}

	inline void ToDegreesMinutesSeconds(double decimalDegrees, int &degrees, int &minutes, int &seconds) {
		double intDegrees, fracDegrees;
		fracDegrees=modf(decimalDegrees, &intDegrees);
		degrees = static_cast<int>(intDegrees);
		fracDegrees *= 60.0;
		double intMinutes, fracMinutes;
		fracMinutes=modf(fracDegrees, &intMinutes);
		minutes = static_cast<int>(intMinutes);
		fracMinutes *= 60.0;
		double intSeconds, fracSeconds;
		fracSeconds=modf(fracMinutes, &intSeconds);
		seconds = static_cast<int>(intSeconds);
		// fracSeconds should be small enough to ignore
	}

	inline void LatToDegreesMinutesSeconds(double decimalDegrees, int &degrees, int &minutes, int &seconds, std::string &hemisphere) {
		ToDegreesMinutesSeconds(fabs(decimalDegrees), degrees, minutes, seconds);
		hemisphere = "";  // 0 has no hemisphere
		if (decimalDegrees>0)
			hemisphere = "N";
		else if (decimalDegrees<0) {
			hemisphere = "S";
		}
	}

	inline void LonToDegreesMinutesSeconds(double decimalDegrees, int &degrees, int &minutes, int &seconds, std::string &hemisphere) {
		ToDegreesMinutesSeconds(fabs(decimalDegrees), degrees, minutes, seconds);
		hemisphere = "";  // 0 has no hemisphere
		if (decimalDegrees>0)
			hemisphere = "E";
		else if (decimalDegrees<0) {
			hemisphere = "W";
		}
	}
}