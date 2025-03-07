#pragma once



#include "InfoLoader.h"
#include "StdString.h"
#include "utils/GlobalsHandling.h"

#include <map>

class TiXmlElement;

#define WEATHER_LABEL_LOCATION   10
#define WEATHER_IMAGE_CURRENT_ICON 21
#define WEATHER_LABEL_CURRENT_COND 22
#define WEATHER_LABEL_CURRENT_TEMP 23
#define WEATHER_LABEL_CURRENT_FEEL 24
#define WEATHER_LABEL_CURRENT_UVID 25
#define WEATHER_LABEL_CURRENT_WIND 26
#define WEATHER_LABEL_CURRENT_DEWP 27
#define WEATHER_LABEL_CURRENT_HUMI 28

struct day_forecast
{
  CStdString m_icon;
  CStdString m_overview;
  CStdString m_day;
  CStdString m_high;
  CStdString m_low;
};

#define NUM_DAYS 7

class CWeatherInfo
{
public:
  day_forecast forecast[NUM_DAYS];

  void Reset()
  {
    lastUpdateTime.clear();
    currentIcon.clear();
    currentConditions.clear();
    currentTemperature.clear();
    currentFeelsLike.clear();
    currentWind.clear();
    currentHumidity.clear();
    currentUVIndex.clear();
    currentDewPoint.clear();

    for (int i = 0; i < NUM_DAYS; i++)
    {
      forecast[i].m_icon.clear();
      forecast[i].m_overview.clear();
      forecast[i].m_day.clear();
      forecast[i].m_high.clear();
      forecast[i].m_low.clear();
    }
  };

  CStdString lastUpdateTime;
  CStdString location;
  CStdString currentIcon;
  CStdString currentConditions;
  CStdString currentTemperature;
  CStdString currentFeelsLike;
  CStdString currentUVIndex;
  CStdString currentWind;
  CStdString currentDewPoint;
  CStdString currentHumidity;
  CStdString busyString;
  CStdString naIcon;
};

class CWeatherJob : public CJob
{
public:
  CWeatherJob(int location);

  virtual bool DoWork();

  const CWeatherInfo &GetInfo() const;
private:
  void LocalizeOverview(CStdString &str);
  void LocalizeOverviewToken(CStdString &str);
  void LoadLocalizedToken();
  int ConvertSpeed(int speed);

  void SetFromProperties();

  /*! \brief Formats a celcius temperature into a string based on the users locale
   \param text the string to format
   \param temp the temperature (in degrees celcius).
   */
  void FormatTemperature(CStdString &text, int temp);

  struct ci_less : std::binary_function<std::string, std::string, bool>
  {
    // case-independent (ci) compare_less binary function
    struct nocase_compare : public std::binary_function<unsigned char,unsigned char,bool>
    {
      bool operator() (const unsigned char& c1, const unsigned char& c2) const {
          return tolower (c1) < tolower (c2);
      }
    };
    bool operator() (const std::string & s1, const std::string & s2) const {
      return std::lexicographical_compare
        (s1.begin (), s1.end (),
        s2.begin (), s2.end (),
        nocase_compare ());
    }
  };

  std::map<CStdString, int, ci_less> m_localizedTokens;
  typedef std::map<CStdString, int, ci_less>::const_iterator ilocalizedTokens;

  CWeatherInfo m_info;
  int m_location;

  static bool m_imagesOkay;
};

class CWeather : public CInfoLoader
{
public:
  CWeather(void);
  virtual ~CWeather(void);
  static bool GetSearchResults(const CStdString &strSearch, CStdString &strResult);

  CStdString GetLocation(int iLocation);
  const CStdString &GetLastUpdateTime() const { return m_info.lastUpdateTime; };
  const day_forecast &GetForecast(int day) const;
  bool IsFetched();
  void Reset();

  void SetArea(int iLocation);
  int GetArea() const;
protected:
  virtual CJob *GetJob() const;
  virtual CStdString TranslateInfo(int info) const;
  virtual CStdString BusyInfo(int info) const;
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

private:

  CWeatherInfo m_info;
};

XBMC_GLOBAL_REF(CWeather, g_weatherManager);
#define g_weatherManager XBMC_GLOBAL_USE(CWeather)
