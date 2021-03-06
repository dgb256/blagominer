#include "common.h"

const unsigned int versionMajor = 1;
const unsigned int versionMinor = 190301;
const unsigned int versionRevision = 0;

// blago version
#ifdef __AVX512F__
std::wstring versionSuffix = L"_AVX512";
#else
#ifdef __AVX2__
std::wstring versionSuffix = L"_AVX2";
#else
#ifdef __AVX__
std::wstring versionSuffix = L"_AVX";
#else
std::wstring versionSuffix = L"_SSE";
//	std::string versionSuffix = "";
#endif
#endif 
#endif 
std::wstring version = std::to_wstring(versionMajor) + L"." + std::to_wstring(versionMinor) + L"." + std::to_wstring(versionRevision) + versionSuffix;

const wchar_t sepChar = 0x00B7;

extern bool lockWindowSize = true;
double checkForUpdateInterval = 1;
bool ignoreSuspectedFastBlocks = true;
volatile bool exit_flag = false;

HANDLE hHeap;

wchar_t *coinNames[] =
{
	(wchar_t *) L"Burstcoin",
	(wchar_t *) L"Bitcoin HD"
};

unsigned long long getHeight(std::shared_ptr<t_coin_info> coin) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mHeight);
	return coin->mining->height;
}
void setHeight(std::shared_ptr<t_coin_info> coin, const unsigned long long height) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mHeight);
	coin->mining->height = height;
}

unsigned long long getTargetDeadlineInfo(std::shared_ptr<t_coin_info> coin) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mTargetDeadlineInfo);
	return coin->mining->targetDeadlineInfo;
}
void setTargetDeadlineInfo(std::shared_ptr<t_coin_info> coin, const unsigned long long targetDeadlineInfo) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mTargetDeadlineInfo);
	coin->mining->targetDeadlineInfo = targetDeadlineInfo;
}

/**
	Don't forget to delete the pointer after using it.
**/
char* getSignature(std::shared_ptr<t_coin_info> coin) {
	char* sig = new char[33];
	RtlSecureZeroMemory(sig, 33);
	{
		std::lock_guard<std::mutex> lockGuard(coin->locks->mSignature);
		memmove(sig, coin->mining->signature, 32);
	}
	return sig;
}
/**
	Don't forget to delete the pointer after using it.
**/
char* getCurrentStrSignature(std::shared_ptr<t_coin_info> coin) {
	char* str_signature = new char[65];
	RtlSecureZeroMemory(str_signature, 65);
	{
		std::lock_guard<std::mutex> lockGuard(coin->locks->mCurrentStrSignature);
		memmove(str_signature, coin->mining->current_str_signature, 64);
	}
	return str_signature;
}

void setSignature(std::shared_ptr<t_coin_info> coin, const char* signature) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mSignature);
	memmove(coin->mining->signature, signature, 32);
}
void setStrSignature(std::shared_ptr<t_coin_info> coin, const char* signature) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mStrSignature);
	memmove(coin->mining->str_signature, signature, 64);
}

void updateOldSignature(std::shared_ptr<t_coin_info> coin) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mSignature);
	std::lock_guard<std::mutex> lockGuardO(coin->locks->mOldSignature);
	memmove(coin->mining->oldSignature, coin->mining->signature, 32);
}

void updateCurrentStrSignature(std::shared_ptr<t_coin_info> coin) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mStrSignature);
	std::lock_guard<std::mutex> lockGuardO(coin->locks->mCurrentStrSignature);
	memmove(coin->mining->current_str_signature, coin->mining->str_signature, 64);
}

bool signaturesDiffer(std::shared_ptr<t_coin_info> coin) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mSignature);
	std::lock_guard<std::mutex> lockGuardO(coin->locks->mOldSignature);
	return memcmp(coin->mining->signature, coin->mining->oldSignature, 32) != 0;
}

bool signaturesDiffer(std::shared_ptr<t_coin_info> coin, const char* sig) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mSignature);
	return memcmp(coin->mining->signature, sig, 32) != 0;
}

bool haveReceivedNewMiningInfo(const std::vector<std::shared_ptr<t_coin_info>>& coins) {
	for (auto& e : coins) {
		std::lock_guard<std::mutex> lockGuard(e->locks->mNewMiningInfoReceived);
		if (e->mining->newMiningInfoReceived) {
			return true;
		}
	}
	return false;
}
void setnewMiningInfoReceived(std::shared_ptr<t_coin_info> coin, const bool val) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mNewMiningInfoReceived);
	coin->mining->newMiningInfoReceived = val;
}

int getNetworkQuality(std::shared_ptr<t_coin_info> coin) {
	if (coin->network->network_quality < 0) {
		return 0;
	}
	return coin->network->network_quality;
}

void getLocalDateTime(const std::time_t &rawtime, char* local, const std::string sepTime) {
	struct tm timeinfo;

	localtime_s(&timeinfo, &rawtime);
	// YYYY-mm-dd HH:MM:SS
	std::string format = "%Y-%m-%d %H" + sepTime + "%M" + sepTime + "%S";
	strftime(local, 80, format.c_str(), &timeinfo);
}

std::wstring toWStr(int number, const unsigned short length) {
	std::wstring s = std::to_wstring(number);
	std::wstring prefix;

	if (length == 0) {
		return L"";
	}
	else if (length == 1 && s.size() > 1) {
		return L">";
	} else if (s.size() > length) {
		s = std::to_wstring(static_cast<int>(pow(10, length - 1)) - 1);
		prefix = L">";
	}
	else if (s.size() < length) {
		prefix = std::wstring(length - s.size(), ' ');
	}
	
	return prefix + s;
}

std::wstring toWStr(unsigned long long number, const unsigned short length) {
	std::string s(toStr(number, length));
	return std::wstring(s.begin(), s.end());
}

std::wstring toWStr(std::wstring str, const unsigned short length) {
	if (str.size() > length) {
		if (length > 3) {
			str = L"..." + std::wstring(str.end() - length + 3, str.end());
		}
		else {
			str = std::wstring(length, '.');
		}
	}
	else if (str.size() < length) {
		str = str + std::wstring(length - str.size(), ' ');
	}
	return str;
}

std::wstring toWStr(std::string str, const unsigned short length) {
	return toWStr(std::wstring(str.begin(), str.end()), length);
}

std::string toStr(unsigned long long number, const unsigned short length) {
	std::string s = std::to_string(number);
	std::string prefix;

	if (length == 0) {
		return "";
	}
	else if (length == 1 && s.size() > 1) {
		return ">";
	}
	else if (s.size() > length) {
		s = std::to_string(static_cast<unsigned long long>(pow(10, length - 1)) - 1);
		prefix = ">";
	}
	else if (s.size() < length) {
		prefix = std::string(length - s.size(), ' ');
	}

	return prefix + s;
}

std::string toStr(std::string str, const unsigned short length) {
	if (str.size() > length) {
		if (length > 3) {
			str = "..." + std::string(str.end() - length + 3, str.end());
		}
		else {
			str = std::string(length, '.');
		}
	}
	else if (str.size() < length) {
		str = str + std::string(length - str.size(), ' ');
	}
	return str;
}