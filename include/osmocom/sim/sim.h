#ifndef _OSMOCOM_SIM_H
#define _OSMOCOM_SIM_H

#include <osmocom/core/msgb.h>
#include <osmocom/core/linuxlist.h>

#define APDU_HDR_LEN	5

/* ISO 7816 / 5.3.1 / Figure 3 + Figure 4 */
enum osim_apdu_case {
	APDU_CASE_1,
	APDU_CASE_2,
	APDU_CASE_2_EXT,
	APDU_CASE_3,
	APDU_CASE_3_EXT,
	APDU_CASE_4,
	APDU_CASE_4_EXT
};

struct osim_apdu_cmd_hdr {
	uint8_t cla;
	uint8_t ins;
	uint8_t p1;
	uint8_t p2;
	uint8_t p3;
} __attribute__ ((packed));

#define msgb_apdu_dr(__x)

struct osim_msgb_cb {
	enum osim_apdu_case apduc;
	uint16_t lc;
	uint16_t le;
	uint16_t sw;
};
#define OSIM_MSGB_CB(__msgb)	((struct osim_msgb_cb *)&((__msgb)->cb[0]))
/*! \brief status word from msgb->cb */
#define msgb_apdu_case(__x)	OSIM_MSGB_CB(__x)->apduc
#define msgb_apdu_lc(__x)	OSIM_MSGB_CB(__x)->lc
#define msgb_apdu_le(__x)	OSIM_MSGB_CB(__x)->le
#define msgb_apdu_sw(__x)	OSIM_MSGB_CB(__x)->sw
/*! \brief pointer to the command header of the APDU */
#define msgb_apdu_h(__x)	((struct osim_apdu_cmd_hdr *)(__x)->l2h)

#define msgb_apdu_dc(__x)	((__x)->l2h + sizeof(struct osim_apdu_cmd_hdr))
#define msgb_apdu_de(__x)	((__x)->l2h + sizeof(struct osim_apdu_cmd_hdr) + msgb_apdu_lc(__x))

struct osim_file;
struct osim_file_desc;
struct osim_decoded_data;

struct osim_file_ops {
	int (*parse)(struct osim_decoded_data *dd,
		     const struct osim_file_desc *desc,
		     int len, uint8_t *data);
	struct msgb * (*encode)(const struct osim_file *file,
				const struct osim_decoded_data *decoded);
};

enum osim_element_type {
	ELEM_T_NONE,
	ELEM_T_BOOL,	/*!< a boolean flag */
	ELEM_T_UINT8,	/*!< unsigned integer */
	ELEM_T_UINT16,	/*!< unsigned integer */
	ELEM_T_UINT32,	/*!< unsigned integer */
	ELEM_T_STRING,	/*!< generic string */
	ELEM_T_BCD,	/*!< BCD encoded digits */
	ELEM_T_BYTES,	/*!< BCD encoded digits */
	ELEM_T_GROUP,	/*!< group container, has siblings */
};

enum osim_element_repr {
	ELEM_REPR_NONE,
	ELEM_REPR_DEC,
	ELEM_REPR_HEX,
};

struct osim_decoded_element {
	struct llist_head list;

	enum osim_element_type type;
	enum osim_element_repr representation;
	const char *name;

	unsigned int length;
	union {
		uint8_t u8;
		uint16_t u16;
		uint32_t u32;
		uint8_t *buf;
		struct llist_head siblings;
	} u;
};

struct osim_decoded_data {
	/*! file to which we belong */
	const struct osim_file *file;
	/*! list of 'struct decoded_element' */
	struct llist_head decoded_elements;
};


enum osim_file_type {
	TYPE_NONE,
	TYPE_DF,
	TYPE_ADF,
	TYPE_EF,
	TYPE_EF_INT,
};

enum osim_ef_type {
	EF_TYPE_TRANSP,
	EF_TYPE_RECORD_FIXED,
	EF_TYPE_RECORD_CYCLIC,
};

#define F_OPTIONAL		0x0001

struct osim_file_desc {
	struct llist_head list;		/*!< local element in list */
	struct llist_head child_list;	/*!< list of children EF in DF */
	struct osim_file_desc *parent;	/*!< parent DF */

	enum osim_file_type type;
	enum osim_ef_type ef_type;

	uint16_t fid;			/*!< File Identifier */
	uint8_t sfid;			/*!< Short File IDentifier */
	const char *df_name;		
	uint8_t df_name_len;

	const char *short_name;		/*!< Short Name (like EF.ICCID) */
	const char *long_name;		/*!< Long / description */
	unsigned int flags;

	struct osim_file_ops ops;
};

struct osim_file {
	const struct osim_file_desc *desc;

	struct msgb *encoded_data;
	struct osim_decoded_data *decoded_data;
};

#define EF(pfid, pns, pflags, pnl, ptype, pdec, penc)	\
	{								\
		.fid		= pfid,					\
		.type		= TYPE_EF,				\
		.ef_type	= ptype,				\
		.short_name	= pns,					\
		.long_name	= pnl,					\
		.flags		= pflags,				\
		.ops 		= { .encode = penc, .parse = pdec },	\
	}


#define EF_TRANSP(fid, ns, flags, nl, dec, enc)	\
		EF(fid, ns, flags, nl, EF_TYPE_TRANSP, dec, enc)
#define EF_TRANSP_N(fid, ns, flags, nl) \
		EF_TRANSP(fid, ns, flags, nl, &default_decode, NULL)

#define EF_CYCLIC(fid, ns, flags, nl, dec, enc)	\
		EF(fid, ns, flags, nl, EF_TYPE_RECORD_CYCLIC, dec, enc)
#define EF_CYCLIC_N(fid, ns, flags, nl) \
		EF_CYCLIC(fid, ns, flags, nl, &default_decode, NULL)

#define EF_LIN_FIX(fid, ns, flags, nl, dec, enc)	\
		EF(fid, ns, flags, nl, EF_TYPE_RECORD_FIXED, dec, enc)
#define EF_LIN_FIX_N(fid, ns, flags, nl)	\
		EF_LIN_FIX(fid, ns, flags, nl, &default_decode, NULL)

struct osim_file_desc *
osim_file_find_name(struct osim_file_desc *parent, const char *name);

enum osim_card_sw_type {
	SW_TYPE_NONE,
	SW_TYPE_STR,
};

enum osim_card_sw_class {
	SW_CLS_NONE,
	SW_CLS_OK,
	SW_CLS_POSTP,
	SW_CLS_WARN,
	SW_CLS_ERROR,
};

struct osim_card_sw {
	uint16_t code;
	uint16_t mask;
	enum osim_card_sw_type type;
	enum osim_card_sw_class class;
	union {
		const char *str;
	} u;
};

#define OSIM_CARD_SW_LAST	{			\
	.code = 0, .mask = 0, .type = SW_TYPE_NONE,	\
	.class = SW_CLS_NONE, .u.str = NULL		\
}

struct osim_card_profile {
	const char *name;
	struct osim_file_desc *mf;
	struct osim_card_sw **sws;
};

extern const struct tlv_definition ts102221_fcp_tlv_def;
const struct value_string ts102221_fcp_vals[14];

/* 11.1.1.3 */
enum ts102221_fcp_tag {
	UICC_FCP_T_FCP		= 0x62,
	UICC_FCP_T_FILE_SIZE	= 0x80,
	UICC_FCP_T_TOT_F_SIZE	= 0x81,
	UICC_FCP_T_FILE_DESC	= 0x82,
	UICC_FCP_T_FILE_ID	= 0x83,
	UICC_FCP_T_DF_NAME	= 0x84,
	UICC_FCP_T_SFID		= 0x88,
	UICC_FCP_T_LIFEC_STS	= 0x8A,
	UICC_FCP_T_SEC_ATTR_REFEXP= 0x8B,
	UICC_FCP_T_SEC_ATTR_COMP= 0x8C,
	UICC_FCP_T_PROPRIETARY	= 0xA5,
	UICC_FCP_T_SEC_ATTR_EXP	= 0xAB,
	UICC_FCP_T_PIN_STS_DO	= 0xC6,
};

struct msgb *osim_new_apdumsg(uint8_t cla, uint8_t ins, uint8_t p1,
			      uint8_t p2, uint16_t lc, uint16_t le);

struct osim_reader_ops;

struct osim_reader_hdl {
	/*! \brief member in global list of readers */
	struct llist_head list;
	struct osim_reader_ops *ops;
	void *priv;
	/*! \brief current card, if any */
	struct osim_card_hdl *card;
};

struct osim_card_hdl {
	/*! \brief member in global list of cards */
	struct llist_head list;
	/*! \brief reader through which card is accessed */
	struct osim_reader_hdl *reader;
	/*! \brief card profile */
	struct osim_card_profile *prof;

	/*! \brief list of channels for this card */
	struct llist_head channels;
};

struct osim_chan_hdl {
	/*! \brief linked to card->channels */
	struct llist_head list;
	/*! \brief card to which this channel belongs */
	struct osim_card_hdl *card;
	const struct osim_file_desc *cwd;
};

/* reader.c */
int osim_transceive_apdu(struct osim_chan_hdl *st, struct msgb *amsg);
struct osim_reader_hdl *osim_reader_open(int idx, const char *name);
struct osim_card_hdl *osim_card_open(struct osim_reader_hdl *rh);
#endif /* _OSMOCOM_SIM_H */
