

#ifndef _XBMC_keytable_h
#define _XBMC_keytable_h

typedef struct struct_XBMCKEYTABLE
{

  // The sym is a value that identifies which key was pressed. Note
  // that it specifies the key not the character so it is unaffected by
  // shift, control, etc.
  uint16_t sym;

  // If the keypress generates a printing character the unicode and
  // ascii member variables contain the character generated. If the
  // key is a non-printing character, e.g. a function or arrow key,
  // the unicode and ascii member variables are zero.
  uint16_t unicode;
  char     ascii;

  // The following two member variables are used to specify the
  // action/function assigned to a key.
  // The keynames are used as tags in keyboard.xml. When reading keyboard.xml
  // TranslateKeyboardString uses the keyname to look up the vkey, and
  // this is used in the mapping table.
  uint32_t    vkey;
  const char* keyname;

} XBMCKEYTABLE;

bool KeyTableLookupName(const char* keyname, XBMCKEYTABLE* keytable);
bool KeyTableLookupSym(uint16_t sym, XBMCKEYTABLE* keytable);
bool KeyTableLookupUnicode(uint16_t unicode, XBMCKEYTABLE* keytable);
bool KeyTableLookupSymAndUnicode(uint16_t sym, uint16_t unicode, XBMCKEYTABLE* keytable);
bool KeyTableLookupVKeyName(uint32_t vkey, XBMCKEYTABLE* keytable);

#endif /* _XBMC_keytable_h */
