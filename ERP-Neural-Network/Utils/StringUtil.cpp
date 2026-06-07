#include "StringUtil.h"

#include "ERP.h"

void StringBuilder::VerifyFit(size_t length)
{
	ERP_VERIFY(CanFit(length));
}

void StringBuilder::Verify(bool condition)
{
	ERP_VERIFY(condition);
}