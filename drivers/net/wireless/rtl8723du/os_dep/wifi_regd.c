// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2009-2010 - 2017 Realtek Corporation */

#include <drv_types.h>
#include <rtw_wifi_regd.h>

/*
 * REG_RULE(freq start, freq end, bandwidth, max gain, eirp, reg_flags)
 */

/*
 *Only these channels all allow active
 *scan on all world regulatory domains
 */

/* 2G chan 01 - chan 11 */
#define RTW_2GHZ_CH01_11	\
	REG_RULE(2412-10, 2462+10, 40, 0, 20, 0)

/*
 *We enable active scan on these a case
 *by case basis by regulatory domain
 */

/* 2G chan 12 - chan 13, PASSIV SCAN */
#define RTW_2GHZ_CH12_13	\
	REG_RULE(2467-10, 2472+10, 40, 0, 20,	\
		 NL80211_RRF_PASSIVE_SCAN)

/* 2G chan 14, PASSIVS SCAN, NO OFDM (B only) */
#define RTW_2GHZ_CH14	\
	REG_RULE(2484-10, 2484+10, 40, 0, 20,	\
		 NL80211_RRF_PASSIVE_SCAN | NL80211_RRF_NO_OFDM)

static const struct ieee80211_regdomain rtw_regdom_rd = {
	.n_reg_rules = 3,
	.alpha2 = "99",
	.reg_rules = {
		RTW_2GHZ_CH01_11,
		RTW_2GHZ_CH12_13,
	}
};

static const struct ieee80211_regdomain rtw_regdom_11 = {
	.n_reg_rules = 1,
	.alpha2 = "99",
	.reg_rules = {
		RTW_2GHZ_CH01_11,
	}
};

static const struct ieee80211_regdomain rtw_regdom_12_13 = {
	.n_reg_rules = 2,
	.alpha2 = "99",
	.reg_rules = {
		RTW_2GHZ_CH01_11,
		RTW_2GHZ_CH12_13,
	}
};

static const struct ieee80211_regdomain rtw_regdom_no_midband = {
	.n_reg_rules = 3,
	.alpha2 = "99",
	.reg_rules = {
		RTW_2GHZ_CH01_11,
	}
};

static const struct ieee80211_regdomain rtw_regdom_60_64 = {
	.n_reg_rules = 3,
	.alpha2 = "99",
	.reg_rules = {
		RTW_2GHZ_CH01_11,
		RTW_2GHZ_CH12_13,
	}
};

static const struct ieee80211_regdomain rtw_regdom_14_60_64 = {
	.n_reg_rules = 4,
	.alpha2 = "99",
	.reg_rules = {
		RTW_2GHZ_CH01_11,
		RTW_2GHZ_CH12_13,
		RTW_2GHZ_CH14,
	}
};

static const struct ieee80211_regdomain rtw_regdom_14 = {
	.n_reg_rules = 3,
	.alpha2 = "99",
	.reg_rules = {
		RTW_2GHZ_CH01_11,
		RTW_2GHZ_CH12_13,
		RTW_2GHZ_CH14,
	}
};

/*
 * Always apply Radar/DFS rules on
 * freq range 5260 MHz - 5700 MHz
 */
static void _rtw_reg_apply_radar_flags(struct wiphy *wiphy)
{
}

static void _rtw_reg_apply_flags(struct wiphy *wiphy)
{
	struct adapter *adapt = wiphy_to_adapter(wiphy);
	struct rf_ctl_t *rfctl = adapter_to_rfctl(adapt);
	struct rt_channel_info *channel_set = rfctl->channel_set;
	u8 max_chan_nums = rfctl->max_chan_nums;

	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *ch;
	unsigned int i, j;
	u16 channel;
	u32 freq;

	/* all channels disable */
	for (i = 0; i < NUM_NL80211_BANDS; i++) {
		sband = wiphy->bands[i];

		if (sband) {
			for (j = 0; j < sband->n_channels; j++) {
				ch = &sband->channels[j];

				if (ch)
					ch->flags = IEEE80211_CHAN_DISABLED;
			}
		}
	}

	/* channels apply by channel plans. */
	for (i = 0; i < max_chan_nums; i++) {
		channel = channel_set[i].ChannelNum;
		freq = rtw_ch2freq(channel);

		ch = ieee80211_get_channel(wiphy, freq);
		if (ch) {
			if (channel_set[i].ScanType == SCAN_PASSIVE) {
				#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0))
				ch->flags = (IEEE80211_CHAN_NO_IBSS | IEEE80211_CHAN_PASSIVE_SCAN);
				#else
				ch->flags = IEEE80211_CHAN_NO_IR;
				#endif
			} else
				ch->flags = 0;
		}
	}
}

static void _rtw_reg_apply_world_flags(struct wiphy *wiphy,
				       enum nl80211_reg_initiator initiator,
				       struct rtw_regulatory *reg)
{
	/* _rtw_reg_apply_beaconing_flags(wiphy, initiator); */
	/* _rtw_reg_apply_active_scan_flags(wiphy, initiator); */
	return;
}

static int _rtw_reg_notifier_apply(struct wiphy *wiphy,
				   struct regulatory_request *request,
				   struct rtw_regulatory *reg)
{

	/* Hard code flags */
	_rtw_reg_apply_flags(wiphy);

	/* We always apply this */
	_rtw_reg_apply_radar_flags(wiphy);

	switch (request->initiator) {
	case NL80211_REGDOM_SET_BY_DRIVER:
		RTW_INFO("%s: %s\n", __func__, "NL80211_REGDOM_SET_BY_DRIVER");
		_rtw_reg_apply_world_flags(wiphy, NL80211_REGDOM_SET_BY_DRIVER,
					   reg);
		break;
	case NL80211_REGDOM_SET_BY_CORE:
		RTW_INFO("%s: %s\n", __func__,
			 "NL80211_REGDOM_SET_BY_CORE to DRV");
		_rtw_reg_apply_world_flags(wiphy, NL80211_REGDOM_SET_BY_DRIVER,
					   reg);
		break;
	case NL80211_REGDOM_SET_BY_USER:
		RTW_INFO("%s: %s\n", __func__,
			 "NL80211_REGDOM_SET_BY_USER to DRV");
		_rtw_reg_apply_world_flags(wiphy, NL80211_REGDOM_SET_BY_DRIVER,
					   reg);
		break;
	case NL80211_REGDOM_SET_BY_COUNTRY_IE:
		RTW_INFO("%s: %s\n", __func__,
			 "NL80211_REGDOM_SET_BY_COUNTRY_IE");
		_rtw_reg_apply_world_flags(wiphy, request->initiator, reg);
		break;
	}

	return 0;
}

static const struct ieee80211_regdomain *_rtw_regdomain_select(struct
		rtw_regulatory
		*reg)
{
	return &rtw_regdom_rd;
}

static void _rtw_reg_notifier(struct wiphy *wiphy, struct regulatory_request *request)
{
	struct rtw_regulatory *reg = NULL;

	RTW_INFO("%s\n", __func__);

	_rtw_reg_notifier_apply(wiphy, request, reg);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 9, 0))
static int rtw_reg_notifier(struct wiphy *wiphy, struct regulatory_request *request)
#else
static void rtw_reg_notifier(struct wiphy *wiphy, struct regulatory_request *request)
#endif
{
	_rtw_reg_notifier(wiphy, request);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 9, 0))
	return 0;
#endif
}

void rtw_reg_notify_by_driver(struct adapter *adapter)
{
	if ((adapter->rtw_wdev) && (adapter->rtw_wdev->wiphy)) {
		struct regulatory_request request;
		request.initiator = NL80211_REGDOM_SET_BY_DRIVER;
		rtw_reg_notifier(adapter->rtw_wdev->wiphy, &request);
	}
}

static void _rtw_regd_init_wiphy(struct rtw_regulatory *reg, struct wiphy *wiphy)
{
	const struct ieee80211_regdomain *regd;

	wiphy->reg_notifier = rtw_reg_notifier;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0))
	wiphy->flags |= WIPHY_FLAG_CUSTOM_REGULATORY;
	wiphy->flags &= ~WIPHY_FLAG_STRICT_REGULATORY;
	wiphy->flags &= ~WIPHY_FLAG_DISABLE_BEACON_HINTS;
#else
	wiphy->regulatory_flags |= REGULATORY_CUSTOM_REG;
	wiphy->regulatory_flags &= ~REGULATORY_STRICT_REG;
	wiphy->regulatory_flags &= ~REGULATORY_DISABLE_BEACON_HINTS;
#endif

	regd = _rtw_regdomain_select(reg);
	wiphy_apply_custom_regulatory(wiphy, regd);

	/* Hard code flags */
	_rtw_reg_apply_flags(wiphy);
	_rtw_reg_apply_radar_flags(wiphy);
	_rtw_reg_apply_world_flags(wiphy, NL80211_REGDOM_SET_BY_DRIVER, reg);
}

int rtw_regd_init(struct adapter *adapt)
{
	struct wiphy *wiphy = adapt->rtw_wdev->wiphy;

	_rtw_regd_init_wiphy(NULL, wiphy);

	return 0;
}
