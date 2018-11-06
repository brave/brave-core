#ifndef BAT_ADS_LOGGING_H_
#define BAT_ADS_LOGGING_H_

#define LOG(client, severity) \
  client->Log(__FILE__, __LINE__, severity)

#endif  // BAT_ADS_LOGGING_H_
