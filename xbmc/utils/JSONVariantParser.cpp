

#include "JSONVariantParser.h"

yajl_callbacks CJSONVariantParser::callbacks = {
  CJSONVariantParser::ParseNull,
  CJSONVariantParser::ParseBoolean,
  CJSONVariantParser::ParseInteger,
  CJSONVariantParser::ParseDouble,
  NULL,
  CJSONVariantParser::ParseString,
  CJSONVariantParser::ParseMapStart,
  CJSONVariantParser::ParseMapKey,
  CJSONVariantParser::ParseMapEnd,
  CJSONVariantParser::ParseArrayStart,
  CJSONVariantParser::ParseArrayEnd
};

CJSONVariantParser::CJSONVariantParser(IParseCallback *callback)
{
  m_callback = callback;

#if YAJL_MAJOR == 2
  m_handler = yajl_alloc(&callbacks, NULL, this);

  yajl_config(m_handler, yajl_allow_comments, 1);
  yajl_config(m_handler, yajl_dont_validate_strings, 0);
#else
  yajl_parser_config cfg = { 1, 1 };

  m_handler = yajl_alloc(&callbacks, &cfg, NULL, this);
#endif

  m_status = ParseVariable;
}

CJSONVariantParser::~CJSONVariantParser()
{
#if YAJL_MAJOR == 2
  yajl_complete_parse(m_handler);
#else
  yajl_parse_complete(m_handler);
#endif
  yajl_free(m_handler);
}

void CJSONVariantParser::push_buffer(const unsigned char *buffer, unsigned int length)
{
  yajl_parse(m_handler, buffer, length);
}

CVariant CJSONVariantParser::Parse(const unsigned char *json, unsigned int length)
{
  CSimpleParseCallback callback;
  CJSONVariantParser parser(&callback);

  parser.push_buffer(json, length);

  return callback.GetOutput();
}

int CJSONVariantParser::ParseNull(void * ctx)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PushObject(CVariant::VariantTypeNull);
  parser->PopObject();

  return 1;
}

int CJSONVariantParser::ParseBoolean(void * ctx, int boolean)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PushObject(CVariant(boolean != 0));
  parser->PopObject();

  return 1;
}

#if YAJL_MAJOR ==2
int CJSONVariantParser::ParseInteger(void * ctx, long long integerVal)
#else
int CJSONVariantParser::ParseInteger(void * ctx, long integerVal)
#endif
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PushObject(CVariant((int64_t)integerVal));
  parser->PopObject();

  return 1;
}

int CJSONVariantParser::ParseDouble(void * ctx, double doubleVal)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PushObject(CVariant((float)doubleVal));
  parser->PopObject();

  return 1;
}

#if YAJL_MAJOR == 2
int CJSONVariantParser::ParseString(void * ctx, const unsigned char * stringVal, size_t stringLen)
#else
int CJSONVariantParser::ParseString(void * ctx, const unsigned char * stringVal, unsigned int stringLen)
#endif
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PushObject(CVariant((const char *)stringVal, stringLen));
  parser->PopObject();

  return 1;
}

int CJSONVariantParser::ParseMapStart(void * ctx)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PushObject(CVariant::VariantTypeObject);

  return 1;
}

#if YAJL_MAJOR == 2
int CJSONVariantParser::ParseMapKey(void * ctx, const unsigned char * stringVal, size_t stringLen)
#else
int CJSONVariantParser::ParseMapKey(void * ctx, const unsigned char * stringVal, unsigned int stringLen)
#endif
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->m_key = std::string((const char *)stringVal, 0, stringLen);

  return 1;
}

int CJSONVariantParser::ParseMapEnd(void * ctx)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PopObject();

  return 1;
}

int CJSONVariantParser::ParseArrayStart(void * ctx)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PushObject(CVariant::VariantTypeArray);

  return 1;
}

int CJSONVariantParser::ParseArrayEnd(void * ctx)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PopObject();

  return 1;
}

void CJSONVariantParser::PushObject(CVariant variant)
{
  if (m_status == ParseObject)
  {
    (*m_parse[m_parse.size() - 1])[m_key] = variant;
    m_parse.push_back(&(*m_parse[m_parse.size() - 1])[m_key]);
  }
  else if (m_status == ParseArray)
  {
    CVariant *temp = m_parse[m_parse.size() - 1];
    temp->push_back(variant);
    m_parse.push_back(&(*temp)[temp->size() - 1]);
  }
  else if (m_parse.size() == 0)
  {
    m_parse.push_back(new CVariant(variant));
  }

  if (variant.isObject())
    m_status = ParseObject;
  else if (variant.isArray())
    m_status = ParseArray;
  else
    m_status = ParseVariable;
}

void CJSONVariantParser::PopObject()
{
  CVariant *variant = m_parse[m_parse.size() - 1];
  m_parse.pop_back();

  if (m_parse.size())
  {
    variant = m_parse[m_parse.size() - 1];
    if (variant->isObject())
      m_status = ParseObject;
    else if (variant->isArray())
      m_status = ParseArray;
    else
      m_status = ParseVariable;
  }
  else if (m_callback)
  {
    m_callback->onParsed(variant);
    delete variant;

    m_parse.clear();
    m_status = ParseVariable;
  }
}
