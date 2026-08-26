#pragma once

/*** Include files ********************************************************************************/

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/
/* C++ code produced by gperf version 2.7.2 */
/* Command-line: '..\\..\\bin\\util\\gperf' -t -K szName -E -D -L C++ -Z SpriteMembersIndex -k 2,6,8 sprite.gperf  */
struct SpriteMembers
{
    const char *szName;
    int nIndex;
};
/* maximum key range = 238, duplicates = 1 */

class SpriteMembersIndex
{
  private:
    static inline unsigned int hash(const char *str, unsigned int len);

  public:
    static struct SpriteMembers *in_word_set(const char *str, unsigned int len);
};
/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/
