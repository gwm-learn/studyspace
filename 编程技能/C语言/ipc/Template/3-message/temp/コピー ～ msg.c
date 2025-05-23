/****************************************************************************
 * NAME         : cp_msgQCreate
 * FUNCTION     : ﾒｯｾｰｼﾞｷｭｰ生成
 * PROCESS      : ﾒｯｾｰｼﾞｷｭｰを生成し、ﾒｯｾｰｼﾞｷｭｰIDを返す
 * INPUT        : Int         msgQKey            : ﾒｯｾｰｼﾞｷｭｰｷｰ
 * OUTPUT       : Int*        msgQId             : ﾒｯｾｰｼﾞｷｭｰID
 * RETURN       : 0:正常
 *              : -1:ｴﾗｰ
 * PROGRAMMED   : T.Yoshimura/CSK
 * DATE         : 05.03.24
 * REMARKS      :
 ****************************************************************************/
Int		cp_msgQCreate(Int msgQKey, Int* msgQId)
{
	Int				msgId			= 0;		/*	ﾒｯｾｰｼﾞｷｭｰID				*/

	/*	ﾒｯｾｰｼﾞｷｭｰ作成			*/
	msgId = msgget(msgQKey, IPC_CREAT | IPC_EXCL | 0666);
	if(msgId == -1){
		printf("[cp_msgQCreate] msgget Error(msgQKey:%d)\n", msgQKey);
		return(-1);
	}

	/*	ﾒｯｾｰｼﾞｷｭｰIDを設定		*/
	*msgQId = msgId;

	return(0);
}


/*********************************************************************
 * NAME         : SetKStat
 * FUNCTION     : 個別負荷状態設定
 * PROCESS      : 個別負荷状態設定
 * INPUT        : liu_no   : LIU番号
 * INPUT        : no       : 個別番号
 * INPUT        : k_stat   : 状態
 * RETURN       : TRUE    : 正常終了 
 *              : FALSE   : 異常終了 
 * PROGRAMMED   : NSS/F.Tokioka(NSS/F.Tokioka)
 * DATE(ORG)    : 2003.03.03(2003.03.03)
 * REMARKS      : 
 ********************************************************************/
Bool SetKStat( unsigned short liu_no, unsigned short no, 
             unsigned char  k_stat)
{
//CIIU BACnet----------
#ifdef BACNET /* CSK YOSHIMURA ADD 2005/05/10 *//* @IFU-MERGE */
    unsigned char	bf_stat;
    unsigned char	level_stat;
    unsigned char	k_level;					/* P009 */
#endif /* CSK YOSHIMURA ADD END */
//----------CIIU BACnet

    /* 共有メモリアタッチ確認 */
    if (!fullInfo)
        return FALSE;

    /* LIU番号チェック */
    if ( liuCheck ( liu_no ) == ERROR )
        return FALSE;

    /* 個別番号チェック */
    if ( kCheck ( no ) == ERROR )
        return FALSE;

    /* セマフォ取得 */
    lockShrMem(getSemId(SEM_K_STAT_KEY));

//CIIU BACnet----------
#ifdef BACNET /* CSK YOSHIMURA ADD 2005/05/10 *//* @IFU-MERGE */
    /* 変更前データ取得 */
    bf_stat = fullInfo->K_Stat[liu_no-1][no-1].Stat;
#endif /* CSK YOSHIMURA ADD END */
//----------CIIU BACnet

    /* データ取得 */
    fullInfo->K_Stat[liu_no-1][no-1].Stat = k_stat;

/*    fullInfo->K_Stat[liu_no-1][no-1].Ctl = 0x00; */

    /* セマフォ解放 */
    unlockShrMem(getSemId(SEM_K_STAT_KEY));

//CIIU BACnet----------
#ifdef BACNET /* CSK YOSHIMURA ADD 2005/05/10 *//* @IFU-MERGE */
    /* 状態変化検知 */
    switch(k_stat){
        case LIU_CTL_OFF:
            k_stat = LIU_CTL_OFF;
            break;
        case LIU_CTL_ON:
        case LIU_CTL_1TIME:
            k_stat = LIU_CTL_ON;
            break;
        default:
            k_stat = LIU_CTL_NON_STAT;
            break;
    }
    switch(bf_stat){
        case LIU_CTL_OFF:
            bf_stat = LIU_CTL_OFF;
            break;
        case LIU_CTL_ON:
        case LIU_CTL_1TIME:
            bf_stat = LIU_CTL_ON;
            break;
        default:
            bf_stat = LIU_CTL_NON_STAT;
            break;
    }
    if(k_stat != bf_stat){						/* P009 */
        SendStatChange(BAC_STAT_CHG, BAC_OBJ_K_STAT, liu_no, no, k_stat);
        /* 調光レベルを取得 */
        GetKLevelVisible(liu_no, no, &level_stat);
        GetKLevel(liu_no, no, &k_level);
        /* 端末異常発生 */
        if(k_stat == LIU_CTL_NON_STAT){
            if(level_stat != 0){
                SendStatChange(BAC_STAT_CHG, BAC_OBJ_K_LEVEL, liu_no, no, BAC_STAT_LEVEL_UNF);
            }
            SendStatChange(BAC_STAT_CHG, BAC_OBJ_K_ERR, liu_no, no, BAC_STAT_ON);
        }
        /* 端末異常復旧 */
        else if(bf_stat == LIU_CTL_NON_STAT){
            if(level_stat != 0){
                SendStatChange(BAC_STAT_CHG, BAC_OBJ_K_LEVEL, liu_no, no, k_level);
            }
            SendStatChange(BAC_STAT_CHG, BAC_OBJ_K_ERR, liu_no, no, BAC_STAT_OFF);
        }
    }
#endif /* CSK YOSHIMURA ADD END */
//----------CIIU BACnet

    return TRUE;
}


/*********************************************************************
 * NAME         : SendStatChange
 * FUNCTION     : 状態変化通知(BACnet)
 * PROCESS      : 状態変化通知取得(BACnet)
 * INPUT        : msg_type	: 通知タイプ
 *              : obj_type	: オブジェクトタイプ
 *              : liu_no	: LIU番号
 *              : no		: 番号
 *              : stat		: 通知する値
 * RETURN       : TRUE    : 正常終了
 *              : FALSE   : 異常終了
 * PROGRAMMED   : CSK YOSHIMURA(CSK YOSHIMURA)
 * DATE(ORG)    : 2005.05.10(2005.05.10)
 * REMARKS      :
 ********************************************************************/
BOOL SendStatChange(int msg_type, unsigned short obj_type, unsigned short liu_no,
					unsigned short no, unsigned char s_stat)
{
	int				rtn;
	int				msgQId;
	unsigned char	g_stat;
	struct {
		int				msg_type;
		unsigned short	obj_type;
		unsigned short	liu_no;
		unsigned short	no;
		unsigned char	dummy[2];
		unsigned char	dat[16];
	} evtdat;

	/* 通知可否チェック */
	GetBACStat(&g_stat);
	if(g_stat == BAC_STAT_NONE){
		return FALSE;
	}

	/* メッセージキューID取得 */
	msgQId = msgget(BAC_MSG_KEY, IPC_EXCL | 0666);
	if(msgQId == -1){
		return FALSE;
	}

	/* メッセージ設定 */
	memset(&evtdat, 0, sizeof(evtdat));
	evtdat.msg_type = msg_type;
	evtdat.obj_type = obj_type;
	evtdat.liu_no   = liu_no;
	evtdat.no       = no;
	evtdat.dat[0]   = s_stat;

	/* メッセージ送信 */
	rtn = msgsnd(msgQId, (char*)&evtdat, sizeof(evtdat), IPC_NOWAIT);
	if(rtn == -1){
		return FALSE;
	}

	return TRUE;
}




/****************************************************************************
 * NAME         : cp_BACSndEvt
 * FUNCTION     : BACnetｲﾍﾞﾝﾄ通知処理
 * PROCESS      : BACnetにｲﾍﾞﾝﾄを通知する
 * INPUT        : EVTDAT*     pEvt               : 送信ﾒｯｾｰｼﾞ
 * RETURN       : 0:正常
 *              : -1:ｴﾗｰ
 * PROGRAMMED   : T.Yoshimura/CSK
 * DATE         : 05.03.24
 * REMARKS      :
 ****************************************************************************/
Int		cp_BACSndEvt(EVTDAT* pEvt)
{
	Int				rtn				= 0;		/*	ﾘﾀｰﾝｺｰﾄﾞ				*/
	Int				msgQId			= 0;		/*	ﾒｯｾｰｼﾞｷｭｰID				*/

	/*	ﾒｯｾｰｼﾞｷｭｰIDを取得		*/
	rtn = cp_msgQGet(KEY_MSG_BAC, &msgQId);
	if(rtn != 0){
		return(-1);
	}

	/*	ﾒｯｾｰｼﾞ送信				*/
	rtn = cp_msgQSend(msgQId, (Char*)pEvt, sizeof(EVTDAT), IPC_NOWAIT);
	if(rtn != 0){
		return(-1);
	}

	return(0);
}



/****************************************************************************
 * NAME         : cp_msgQCreate
 * FUNCTION     : ﾒｯｾｰｼﾞｷｭｰ生成
 * PROCESS      : ﾒｯｾｰｼﾞｷｭｰを生成し、ﾒｯｾｰｼﾞｷｭｰIDを返す
 * INPUT        : Int         msgQKey            : ﾒｯｾｰｼﾞｷｭｰｷｰ
 * OUTPUT       : Int*        msgQId             : ﾒｯｾｰｼﾞｷｭｰID
 * RETURN       : 0:正常
 *              : -1:ｴﾗｰ
 * PROGRAMMED   : T.Yoshimura/CSK
 * DATE         : 05.03.24
 * REMARKS      :
 ****************************************************************************/
Int		cp_msgQCreate(Int msgQKey, Int* msgQId)
{
	Int				msgId			= 0;		/*	ﾒｯｾｰｼﾞｷｭｰID				*/

	/*	ﾒｯｾｰｼﾞｷｭｰ作成			*/
	msgId = msgget(msgQKey, IPC_CREAT | IPC_EXCL | 0666);
	if(msgId == -1){
		printf("[cp_msgQCreate] msgget Error(msgQKey:%d)\n", msgQKey);
		return(-1);
	}

	/*	ﾒｯｾｰｼﾞｷｭｰIDを設定		*/
	*msgQId = msgId;

	return(0);
}

/****************************************************************************
 * NAME         : cp_msgQGet
 * FUNCTION     : ﾒｯｾｰｼﾞｷｭｰID取得
 * PROCESS      : ﾒｯｾｰｼﾞｷｭｰIDを取得し、返す
 * INPUT        : Int         msgQKey            : ﾒｯｾｰｼﾞｷｭｰｷｰ
 * OUTPUT       : Int*        msgQId             : ﾒｯｾｰｼﾞｷｭｰID
 * RETURN       : 0:正常
 *              : -1:ｴﾗｰ
 * PROGRAMMED   : T.Yoshimura/CSK
 * DATE         : 05.03.24
 * REMARKS      :
 ****************************************************************************/
Int		cp_msgQGet(Int msgQKey, Int* msgQId)
{
	Int				msgId			= 0;		/*	ﾒｯｾｰｼﾞｷｭｰID				*/

	/*	ﾒｯｾｰｼﾞｷｭｰ作成			*/
	msgId = msgget(msgQKey, IPC_EXCL | 0666);
	if(msgId == -1){
		return(-1);
	}

	/*	ﾒｯｾｰｼﾞｷｭｰIDを設定		*/
	*msgQId = msgId;

	return(0);
}

/****************************************************************************
 * NAME         : cp_msgQDelete
 * FUNCTION     : ﾒｯｾｰｼﾞｷｭｰ削除
 * PROCESS      : 指定されたﾒｯｾｰｼﾞｷｭｰを削除する
 * INPUT        : Int         msgQId             : ﾒｯｾｰｼﾞｷｭｰID
 * RETURN       : 0:正常
 *              : -1:ﾒｯｾｰｼﾞｷｭｰ削除ｴﾗｰ
 * PROGRAMMED   : T.Yoshimura/CSK
 * DATE         : 05.03.24
 * REMARKS      :
 ****************************************************************************/
Int		cp_msgQDelete(Int msgQId)
{
	Int				rtn				= 0;		/*	ﾘﾀｰﾝｺｰﾄﾞ				*/

	/*	ﾒｯｾｰｼﾞｷｭｰを削除			*/
	rtn = msgctl(msgQId, IPC_RMID, NULL);
	if(rtn != 0){
		return(-1);
	}

	return(0);
}

/****************************************************************************
 * NAME         : cp_msgQSend
 * FUNCTION     : ﾒｯｾｰｼﾞ送信
 * PROCESS      : 指定されたﾒｯｾｰｼﾞｷｭｰIDにﾒｯｾｰｼﾞを送信する
 * INPUT        : Int         msgQId             : ﾒｯｾｰｼﾞｷｭｰID
 *              : Char*       buffer             : 送信ﾒｯｾｰｼﾞ
 *              : Int         msgsize            : 送信ｻｲｽﾞ
 *              : Int         msgflg             : 送信ﾌﾗｸﾞ(0, IPC_NOWAIT)
 * RETURN       : 0:正常
 *              : -1:msg_qbytes制限ｴﾗｰ
 *              : -2:その他ｴﾗｰ
 * PROGRAMMED   : T.Yoshimura/CSK
 * DATE         : 05.03.24
 * REMARKS      :
 ****************************************************************************/
Int		cp_msgQSend(Int msgQId, Char* buffer, Int msgsize, Int msgflg)
{
	Int				rtn				= 0;		/*	ﾘﾀｰﾝｺｰﾄﾞ				*/

	/*	ﾒｯｾｰｼﾞを送信			*/
	rtn = msgsnd(msgQId, buffer, msgsize, msgflg);
	if(rtn != 0){
		if(errno == EAGAIN){
			rtn = -1;
		}
		else{
			rtn = -2;
		}
		printf("[cp_msgQSend] msgsnd Error(msgQId:%d rtn:%d errno:%d)\n", msgQId, rtn, errno);
		return(rtn);
	}

	return(0);
}

/****************************************************************************
 * NAME         : cp_msgQReceive
 * FUNCTION     : ﾒｯｾｰｼﾞ受信
 * PROCESS      : 指定されたﾒｯｾｰｼﾞｷｭｰIDのﾒｯｾｰｼﾞを受信する
 * INPUT        : Int         msgQId             : ﾒｯｾｰｼﾞｷｭｰID
 *              : Char*       buffer             : 受信ﾒｯｾｰｼﾞ
 *              : Int         msgsize            : 受信ｻｲｽﾞ
 *              : Int         msgflg             : 受信ﾌﾗｸﾞ(0, IPC_NOWAIT)
 * RETURN       : 0:正常
 *              : -1:ﾒｯｾｰｼﾞｷｭｰ未生成ｴﾗｰ
 *              : -2:ﾒｯｾｰｼﾞ受信ｴﾗｰ
 * PROGRAMMED   : T.Yoshimura/CSK
 * DATE         : 05.03.24
 * REMARKS      :
 ****************************************************************************/
Int		cp_msgQReceive(Int msgQId, Char* buffer, Int msgsize, Int msgflg)
{
	Int				rtn				= 0;		/*	ﾘﾀｰﾝｺｰﾄﾞ				*/

	/*	受信領域をﾁｪｯｸ			*/
	if(buffer == NULL){
		return(-1);
	}

	/*	ﾒｯｾｰｼﾞを受信			*/
	rtn = msgrcv(msgQId, buffer, msgsize, 0, msgflg);
	if(rtn <= 0){
		printf("[cp_msgQReceive] msgrcv Error(msgQId:%d rtn:%d errno:%d msgsize:%d)\n", msgQId, rtn, errno, msgsize);
		return(-1);
	}

	return(0);
}


/****************************************************************************
 * NAME     : BFLmsgQNumMsgs
 * FUNCTION : メッセージキューに登録してあるメッセージ数を取得する。
 * ARG1     : メッセージキューIDポインタ
 * RET      : メッセージ数(-1:ERROR)
 * NOTE     : 
 ***************************************************************************/
int BFLmsgQNumMsgs(MSG_Q_ID msgQId)
{
	int rval;
	struct msqid_ds buf;

	if (msgQId == NULL) return (ERROR);
//printf("# BFLmsgQNumMsgs Start:%d: #\n", *msgQId);   /* DEBUG */
	rval = msgctl(*msgQId, IPC_STAT, &buf);
	if (rval != ERROR) {
//printf("# BFLmsgQNumMsgs2 #\n");   /* DEBUG */
		rval = buf.msg_qnum;
	}
//printf("# BFLmsgQNumMsgs3 #\n");   /* DEBUG */
	if (BFLinfo & BFLINF_MSG) {
		printf("### BFLmsgQNumMsgs:%d: msg_cnt=%d ###\n", *msgQId, rval);
	}
//printf("# BFLmsgQNumMsgs End:%d: #\n", *msgQId);   /* DEBUG */

	return (rval);
}



/* hzg
msgsz = sizeof(struct mymsgbuf) - sizeof(long);



//test

struct msg {
	struct 		msg *msg_next; 			// next message on queue 
	long 		msg_type;
	char 		*msg_spot; 				// message text address 
	time_t 		msg_stime; 				// msgsnd time 
	short 		msg_ts; 				// message text size
};


int open_queue( key_t keyval )
{
	int qid;
	if((qid = msgget( keyval, IPC_CREAT | 0660 )) == -1)
	{
		return(-1);
	}
	return(qid);
}



int send_message( int qid, struct mymsgbuf *qbuf )
{
	int result, length;
	
	// The length is essentially the size of the structure minus sizeof(mtype)
	length = sizeof(struct mymsgbuf) - sizeof(long);
	if((result = msgsnd( qid, qbuf, length, 0)) == -1)
	{
		return(-1);
	}
	return(result);
}


main()
{
	int qid;
	key_t msgkey;
	struct mymsgbuf
	{
		long mtype; 				// Message type 
		int request; 				// Work request number
		double salary; 				// Employee's salary
	} msg;
	
	// Generate our IPC key value
	msgkey = ftok(".", 'm');
	
	// Open/create the queue
	if(( qid = open_queue( msgkey)) == -1) 
	{
		perror("open_queue");
		exit(1);
	}
	
	// Load up the message with arbitrary test data 
	msg.mtype = 1;					// Message type must be a positive number! 
	msg.request = 1; 				// Data element #1 
	msg.salary = 1000.00; 			// Data element #2 (my yearly salary!) 
	
	// Bombs away! 
	if((send_message( qid, &msg )) == -1) 
	{
		perror("send_message");
		exit(1);
	}
}











int read_message( int qid, long type, struct mymsgbuf *qbuf )
{
int result, length;
/* The length is essentially the size of the structure minus sizeof(mtype) */
length = sizeof(struct mymsgbuf) - sizeof(long);
if((result = msgrcv( qid, qbuf, length, type, 0)) == -1)
{
return(-1);
}
return(result);
}

*/