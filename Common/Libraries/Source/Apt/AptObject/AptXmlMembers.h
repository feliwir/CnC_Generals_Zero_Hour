#pragma once

/* C++ code produced by gperf version 2.7.2 */
/* Command-line: '..\\..\\bin\\util\\gperf.exe' -t -K szName -E -L C++ -Z XmlMemberIndex -k 2,6,8 xml.gperf  */
struct XmlMembers
{
    char *szName;
    int nIndex;
};
/* maximum key range = 73, duplicates = 0 */

class XmlMemberIndex
{
  private:
    static inline unsigned int hash(const char *str, unsigned int len);

  public:
    static struct XmlMembers *in_word_set(const char *str, unsigned int len);
};
