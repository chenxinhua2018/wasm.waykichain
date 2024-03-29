// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(main_tests)

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{

	uint64_t nSum = 0;
	if(MAIN_NET == SysCfg().NetworkID()) {
		for (int height = 0; height < 14000000; height += 1000) {
			uint64_t nSubsidy = GetBlockValue(height, 0);
			BOOST_CHECK(nSubsidy <= 50 * COIN);
			nSum += nSubsidy * 1000;
			BOOST_CHECK(CheckBaseCoinRange(nSum));
		}
		printf("%llu\n", (unsigned long long)nSum);
		BOOST_CHECK(nSum == 2099999997690000ULL);
	}
	else
	{
//		uint64_t sSum = 0;
		for(int height = 0; height < 10000; height+=150) {
			uint64_t nSubsidy = GetBlockValue(height, 0);
			BOOST_CHECK(nSubsidy <= 50 * COIN);
			nSum += nSubsidy * 150;
			BOOST_CHECK(CheckBaseCoinRange(nSum));
		}
		BOOST_CHECK(nSum == 5357138153850ULL);
	}
}

BOOST_AUTO_TEST_SUITE_END()
