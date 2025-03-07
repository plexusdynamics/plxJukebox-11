#pragma once



#include <vector>

#define DIFFRINGSIZE 120

class CPullupCorrection
{
  public:
    CPullupCorrection();
    void   Add(double pts);
    void   Flush(); //flush the saved pattern and the ringbuffer

    double GetCorrection()    { return m_ptscorrection;            }
    int    GetPatternLength() { return m_patternlength;            }
    double GetFrameDuration() { return m_frameduration;            }
    bool   HasFullBuffer()    { return m_ringfill == DIFFRINGSIZE; }

  private:
    double m_prevpts;                //last pts added
    double m_diffring[DIFFRINGSIZE]; //ringbuffer of differences between pts'
    int    m_ringpos;                //position of last diff added to ringbuffer
    int    m_ringfill;               //how many diffs we have in the ringbuffer
    double GetDiff(int diffnr);      //gets diffs from now to the past

    void GetPattern(std::vector<double>& pattern);     //gets the current pattern
    void GetDifftypes(std::vector<double>& difftypes); //gets the difftypes from the ringbuffer

    bool MatchDiff(double diff1, double diff2); //checks if two diffs match by MAXERR
    bool MatchDifftype(int* diffs1, int* diffs2, int nrdiffs); //checks if the difftypes match

    //builds a pattern of timestamps in the ringbuffer
    void BuildPattern(std::vector<double>& pattern, int patternlength);

    //checks if the current pattern matches with the saved m_pattern with offset m_patternpos
    bool CheckPattern(std::vector<double>& pattern);

    double CalcFrameDuration(); //calculates the frame duration from m_pattern

    std::vector<double> m_pattern; //the last saved pattern
    int    m_patternpos;           //the position of the pattern in the ringbuffer, moves one to the past each time a pts is added
    double m_ptscorrection;        //the correction needed for the last added pts
    double m_trackingpts;          //tracked pts for smoothing the timestamps
    double m_frameduration;        //frameduration exposed to dvdplayer, used for calculating the fps
    bool   m_haspattern;           //for the log
    int    m_patternlength;        //for the codec info
    CStdString GetPatternStr();    //also for the log
};
