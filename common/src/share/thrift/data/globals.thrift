struct MJAPICommon
{
	1:required string type,
	2:required string qid,
	3:optional string uuid,
	4:optional string uid,
	5:optional string token,
	6:optional i32 dev,
	7:optional string ver,
	8:optional string lang="zh_cn",
	9:optional string ccy="CNY",
	10:optional string channel,
	11:optional i32 net,
	12:optional string csuid,
	13:optional string ptid
}

struct MJRequest
{
	1:required MJAPICommon api,
	2:optional string query
}



struct MJError
{
	1:i32 error_id,
	2:optional string error_str,
	3:optional list<string> error_param,
	4:optional string error_reason,
	5:optional string error_url
}


