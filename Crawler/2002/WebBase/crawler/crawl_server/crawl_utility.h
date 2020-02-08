#ifndef CRAWL_UTILITY_H
#define CRAWL_UTILITY_H

// status file - <wlam@cs.stanford.edu> Feb 2001
void CreateStatusFile(void);
void FinishedPage(const int numbytes);
void IncrementSites(void);
void DecrementSites(void);
char *ReportStatus(void);
      
#endif // CRAWL_UTILITY
