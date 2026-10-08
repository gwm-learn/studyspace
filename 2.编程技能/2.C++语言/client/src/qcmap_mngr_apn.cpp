#include "qcmap_mngr_apn.h"
#include "qcmap_mngr_info.h"
#include "qcmap_mngr_lock.h"
#include "qcmap_mngr_modem.h"
#include "qcmap_mngr_config.h"

static provider_t providers_LTE[] = {
    // MCCMNC      number       apn                              username                         password
    {"21630", "*99#", "internet.telekom", "", ""},
    {"2191", "*99#", "Internet.ht.hr", "", ""},
    {"2191", "*99#", "internet.ht.hr", "", ""},
    {"2192", "*99#", "internet.tele2.hr", "", ""},
    {"2192", "*99#", "internet.tele2.hr", "", ""},
    {"21910", "*99#", "tomato", "", ""},
    {"21910", "*99#", "data.vip.hr", "38591", "38591"},
    {"21910", "*99#", "gprs0.vipnet.hr", "38591", "38591"},
    {"2408", "", "internet.telenor.se", "", ""},
    {"25099", "*99#", "internet.beeline.ru", "beeline", "beeline"},
    {"25010", "*99#", "internet.mts.ru", "mts", "mts"},
    {"2502", "*99#", "internet", "gdata", "gdata"},
    {"2502", "*99#", "internet.ltmsk", "gdata", "gdata"},
    {"25020", "*99#", "internet.tele2.ru", "", ""},
    {"2505", "*99#", "internet.smarts.ru", "", ""},
    {"2505", "*99#", "inet.bwc.ru", "bwc", "bwc"},
    {"25011", "*99#", "internet.yota", "", ""},
    {"25054", "*99#", "internet.letai.ru", "", ""},
    {"28413", "*99#", "apn.maxtelecom.bg", "", ""},
    {"28411", "*99#", "bulsat.com", "", ""},
    {"2845", "*99#", "telenorbg", "", ""},
    {"2841", "*99#", "inet-gprs.mtel.bg", "", ""},
    {"29340", "*99#", "internet.simobil.si", "simobil", "internet"},
    {"29341", "*99#", "internet", "mobitel", "internet"},
    {"29370", "*99#", "internet.tusmobil.si", "tusmobil", "internet"},
    {"29364", "*99#", "internet.t-2.net", "", ""},
    {"29370", "*99#", "telemach.net", "telemach", "internet"},
    {"2941", "*99#", "internet", "internet", "t-mobile"},
    {"2943", "*99#", "vipoperator", "vipoperator", "vipoperator"},
    {"2972", "*99#", "tmcg-mnw", "38267", "38267"},
    {"2971", "*99#", "flat", "gprs", "gprs"},
    {"310012", "", "vzwinternet", "", ""},
    {"311580", "", "usccinternet", "", ""},
    {"40449", "", "airtelgprs.com", "", ""},
    {"40440", "", "airtelgprs.com", "", ""},
    {"40410", "", "airtelgprs.com", "", ""},
    {"40498", "", "airtelgprs.com", "", ""},
    {"40496", "", "airtelgprs.com", "", ""},
    {"40403", "", "airtelgprs.com", "", ""},
    {"40445", "", "airtelgprs.com", "", ""},
    {"40495", "", "airtelgprs.com", "", ""},
    {"40431", "", "airtelgprs.com", "", ""},
    {"40493", "", "airtelgprs.com", "", ""},
    {"40490", "", "airtelgprs.com", "", ""},
    {"40492", "", "airtelgprs.com", "", ""},
    {"40416", "", "airtelgprs.com", "", ""},
    {"40402", "", "airtelgprs.com", "", ""},
    {"40470", "", "airtelgprs.com", "", ""},
    {"40494", "", "airtelgprs.com", "", ""},
    {"40497", "", "airtelgprs.com", "", ""},
    {"40404", "", "internet", "", ""},
    {"40424", "", "internet", "", ""},
    {"40412", "", "internet", "", ""},
    {"40482", "", "internet", "", ""},
    {"40444", "", "internet", "", ""},
    {"40419", "", "internet", "", ""},
    {"40478", "", "internet", "", ""},
    {"40422", "", "internet", "", ""},
    {"40470", "", "internet", "", ""},
    {"40494", "", "internet", "", ""},
    {"40497", "", "internet", "", ""},
    {"40413", "", "www", "", ""},
    {"40484", "", "www", "", ""},
    {"40411", "", "www", "", ""},
    {"40405", "", "www", "", ""},
    {"40401", "", "www", "", ""},
    {"40486", "", "www", "", ""},
    {"40446", "", "www", "", ""},
    {"40430", "", "www", "", ""},
    {"40427", "", "www", "", ""},
    {"40420", "", "www", "", ""},
    {"40460", "", "www", "", ""},
    {"40443", "", "www", "", ""},
    {"40415", "", "www", "", ""},
    {"40488", "", "www", "", ""},
    {"405854", "", "jionet", "", ""},
    {"405855", "", "jionet", "", ""},
    {"405856", "", "jionet", "", ""},
    {"405872", "", "jionet", "", ""},
    {"405857", "", "jionet", "", ""},
    {"405858", "", "jionet", "", ""},
    {"405859", "", "jionet", "", ""},
    {"405860", "", "jionet", "", ""},
    {"405861", "", "jionet", "", ""},
    {"405862", "", "jionet", "", ""},
    {"405873", "", "jionet", "", ""},
    {"405863", "", "jionet", "", ""},
    {"405864", "", "jionet", "", ""},
    {"405874", "", "jionet", "", ""},
    {"405865", "", "jionet", "", ""},
    {"405866", "", "jionet", "", ""},
    {"405867", "", "jionet", "", ""},
    {"405868", "", "jionet", "", ""},
    {"405869", "", "jionet", "", ""},
    {"405871", "", "jionet", "", ""},
    {"405870", "", "jionet", "", ""},
    {"405840", "", "jionet", "", ""},
    {"40556", "", "airtelgprs.com", "", ""},
    {"40552", "", "airtelgprs.com", "", ""},
    {"40555", "", "airtelgprs.com", "", ""},
    {"40553", "", "airtelgprs.com", "", ""},
    {"40554", "", "airtelgprs.com", "", ""},
    {"40551", "", "airtelgprs.com", "", ""},
    {"405908", "", "internet", "", ""},
    {"405845", "", "internet", "", ""},
    {"40570", "", "internet", "", ""},
    {"405909", "", "internet", "", ""},
    {"405910", "", "internet", "", ""},
    {"405846", "", "internet", "", ""},
    {"405847", "", "internet", "", ""},
    {"405848", "", "internet", "", ""},
    {"405911", "", "internet", "", ""},
    {"405799", "", "internet", "", ""},
    {"40554", "", "internet", "", ""},
    {"40551", "", "internet", "", ""},
    {"405824", "", "www", "", ""},
    {"405751", "", "www", "", ""},
    {"405752", "", "www", "", ""},
    {"405827", "", "www", "", ""},
    {"405754", "", "www", "", ""},
    {"405750", "", "www", "", ""},
    {"405834", "", "www", "", ""},
    {"405756", "", "www", "", ""},
    {"405755", "", "www", "", ""},
    {"405753", "", "www", "", ""},
    {"40566", "", "www", "", ""},
    {"40567", "", "www", "", ""},
    {"4102", "*99#", "eagle.com", "vwireless@eagle.com", "ptcl"},
    {"4272", "*99#", "web.vodafone.com.qa", "", ""},
    {"4271", "*99#", "data", "", ""},
    {"42901", "", "ntnet", "", ""},
    {"42902", "", "web", "", ""},
    {"440", "*99#", "mopera.net", "", ""},
    {"45006", "", "internet.lguplus.co.kr", "", ""},
    {"46605", "*99#", "genet", "", ""},
    {"50216", "*99#", "3gdgnet", "", ""},
    {"502153", "", "webe", "", ""},
    {"502152", "", "yesnet", "", ""},
    {"5051", "*99#", "telstra.internet", "", ""},
    {"5052", "*99#", "yesinternet", "", ""},
    {"5053", "*99#", "live.vodafone.com", "", ""},
    {"5056", "*99#", "3netaccess", "", ""},
    {"50514", "*99#", "vfinternet.au", "", ""},
    {"5052", "*99#", "Internet", "", ""},
    {"5051", "*99#", "telstra.bigpond", "", ""},
    {"5052", "*99#", "connect", "", ""},
    {"50538", "*99#", "purtona.net", "", ""},
    {"5052", "*99#", "4GLTE", "", ""},
    {"5052", "*99#", "CONNECT", "", ""},
    {"5052", "*99#", "connect", "", ""},
    {"5052", "*99#", "internet", "", ""},
    {"5052", "*99#", "primuslns1", "", ""},
    {"5052", "*99#", "internet", "", ""},
    {"5052", "*99#", "VirginBroadband", "", ""},
    {"5109", "", "smartfren4g", "smartfren", "smartfren"},
    {"51088", "", "internet", "", ""},
    {"52001", "*99#", "internet", "", ""},
    {"5251", "*99#", "e-ideas", "", ""},
    {"5255", "*99#", "shwap", "", ""},
    {"5301", "*99#", "www.vodafone.net.nz", "", ""},
    {"5305", "*99#", "internet.telecom.co.nz", "", ""},
    {"53024", "*99#", "internet", "", ""},
    {"5305", "*99#", "www.orcon.net.nz", "", ""},
    {"5305", "*99#", "www.callplus.net.nz", "", ""},
    {"712", "", "kolbi3g", "", ""},
    {"7166", "*99#", "movistar.pe", "", ""},
    {"716", "*99#", "nextel.pe", "", ""},
    {"7166", "*99#", "movistar.pe", "movistar@datos", "movistar"},
    {"71617", "*99#", "nextel.pe", "", ""},
    {"7242", "*99#", "Tim.br", "tim", "tim"},
    {"7243", "*99#", "Tim.br", "tim", "tim"},
    {"7244", "*99#", "Tim.br", "tim", "tim"},
    {"7245", "*99***1#", "bandalarga.claro.com.br", "claro", "claro"},
    {"7246", "*99#", "zap.vivo.com.br", "vivo", "vivo"},
    {"72410", "*99#", "zap.vivo.com.br", "vivo", "vivo"},
    {"72411", "*99#", "zap.vivo.com.br", "vivo", "vivo"},
    {"72423", "*99#", "zap.vivo.com.br", "vivo", "vivo"},
    {"72431", "*99#", "gprs.oi.com.br", "oi", "oi"},
    {"72434", "*99#", "ctbc.br", "ctbc", "1212"},
    {"72439", "*99#", "wap.nextel3g.net.br", "", ""},
    {"7301", "*99#", "imovil.entelpcs.cl", "entelpcs", "entelpcs"},
    {"73010", "*99#", "imovil.entelpcs.cl", "entelpcs", "entelpcs"},
    {"7302", "*99#", "web.tmovil.cl", "web", "web"},
    {"7303", "*99#", "bam.clarochile.cl", "clarochile", "clarochile"},
    {"7303", "*99#", "bap.clarochile.cl", "clarochile", "clarochile"},
    {"732142", "*99#", "une4glte.net.co", "", ""},
    {"732", "*99#", "", "", ""},
    {"732130", "*99#", "", "", ""},
    {"732187", "*99#", "moviletb.net.co", "etb", "etb"},
    {"732", "*99#", "", "", ""},
    {"732176", "*99#", "", "", ""},
    {"732142", "*99#", "une4glte.net.co", "une", "une"},
    {"7362", "*99#", "4g.entel", "", ""},
    {"7362", "*99#", "4g.entel", "", ""},
    {"62160", "*99#", "9mobile", "", ""},
    {"62150", "*99#", "glosecure", "secure", "secure"},
    {"62127", "*99#", "internet", "", ""},
    {"62126", "*99#", "lte.swiftng.com", "", ""},
    {"62124", "*99#", "spectranet", "", ""},
    {"62130", "*99#", "web.gprs.mtnnigeria.net", "web", "web"},
    {"62140", "*99#", "ntel", "ntel", "ntel"},
    {"62140", "*99#", "ntel", "", ""},
    {"62120", "*99#", "internet.ng.airtel.com", "", ""},
    {"62150", "*99#", "APN", "Flat", "Flat"},
    {"6201", "*99#", "internet", "", ""},
    {"6202", "*99#", "Browse/internet", "", ""},
    {"6203", "*99#", "web.tigo.com.gh", "", ""},
    {"6203", "*99#", "web.tigo.com.gh", "", ""},
    {"6207", "*99#", "internet", "", ""},
    {"6208", "*99#", "Internet", "", ""},
    {"6202", "*99#", "internet", "", ""},
    {"46001", "*99#", "3gnet", "any", "any"},
    {"46006", "*99#", "3gnet", "any", "any"},
    {"46003", "*99#", "ctnet", "ctnet@mycdma.cn", "vnet.mobi"},
    {"46005", "*99#", "ctnet", "ctnet@mycdma.cn", "vnet.mobi"},
    {"46011", "*99#", "ctnet", "ctnet@mycdma.cn", "vnet.mobi"},
    {"46060", "*99#", "cbnet", "any", "any"},
    {"46000", "*99***1#", "cmnet", "any", "any"},
    {"46004", "*99***1#", "cmnet", "any", "any"},
    {"46007", "*99***1#", "cmnet", "any", "any"},
    {"46002", "*99***1#", "cmnet", "any", "any"},
    {"51502", "", "athome.globe.com.ph", "", ""},
    {"23410", "*99#", "mobile.o2.co.uk", "o2web", "password"},
    {"23410", "*99#", "payandgo.o2.co.uk", "payandgo", "password"},
    {"23415", "*99#", "pp.vodafone.co.uk", "wap", "wap"},
    {"23415", "*99#", "internet", "web", "web"},
    {"23420", "*99#", "3internet", "", ""},
    {"23420", "*99#", "three.co.uk", "", ""},
    {"23430", "*99#", "everywhere", "eesecure", "secure"},
};

static provider_t providers_3G[] = {
    // MCCMNC	  number	   apn	  	  	  	  	  	  	    username	  	  	  	  	  	 password
    {"2021", "*99#", "internet", "", ""},
    {"2025", "*99#", "internet.vodafone.gr", "", ""},
    {"20210", "*99#", "gint.b-online.gr", "web", "web"},
    {"2042", "*99#", "data.tele2.nl", "", ""},
    {"2044", "*99#", "live.vodafone.com", "vodafone", "vodafone"},
    {"2044", "*99#", "office.vodafone.nl", "vodafone", "vodafone"},
    {"2048", "*99#", "internet", "KPN", "gprs"},
    {"20412", "*99#", "internet", "", ""},
    {"20416", "*99#", "internet", "", ""},
    {"20420", "*99#", "internet", "orange", "orange"},
    {"2042300", "*99#", "data.tele2.nl", "", ""},
    {"2061", "*99#", "internet.proximus.be", "", ""},
    {"20610", "*99#", "mworld.be", "", ""},
    {"20610", "*99#", "iew.be", "mobistar", "mobistar"},
    {"20610", "*99#", "internet.be", "mobistar", "mobistar"},
    {"20610", "*99#", "web.pro.be", "mobistar", "mobistar"},
    {"20620", "*99#", "gprs.base.be", "base", "base"},
    {"20620", "*99#", "busint.base.be", "BusinessInternet", "busint"},
    {"2081", "*99#", "orange.fr", "orange", "orange"},
    {"2081", "*99#", "ofnew.fr", "orange", "orange"},
    {"2081", "*99#", "Wapmms", "orange", "orange"},
    {"2081", "*99#", "wap68", "orange", "orange"},
    {"2081", "*99#", "virgin-mobile.fr", "orange", "orange"},
    {"2081", "*99#", "orange.acte", "orange", "orange"},
    {"2081", "*99#", "ofnew.fr", "orange", "orange"},
    {"20810", "*99#", "wapsfr", "", ""},
    {"20810", "*99#", "wap66", "", ""},
    {"20810", "*99#", "sl2sfr", "", ""},
    {"20810", "*99#", "sl2sfr", "", ""},
    {"20815", "*99#", "free", "", ""},
    {"20820", "*99#", "a2bouygtel.com", "", ""},
    {"20820", "*99#", "b2bouygtel.com", "", ""},
    {"20820", "*99#", "fipbouygtel.com", "", ""},
    {"20820", "*99#", "vpnbouygtel.com", "", ""},
    {"20820", "*99#", "mmsbouygtel.com", "", ""},
    {"2141", "*99#", "ac.vodafone.es", "vodafone", "vodafone"},
    {"2143", "*99#", "orangeworld", "orange", "orange"},
    {"2143", "*99#", "orangeworld", "orange", "orange"},
    {"2143", "*99#", "inet.es", "", ""},
    {"2143", "*99#", "internetmas", "", ""},
    {"2143", "*99#", "internet.euskaltel.mobi", "CLIENTE", "EUSKALTEL"},
    {"2143", "*99#", "internet.racc.net", "CLIENTERACC", "RACC"},
    {"2143", "*99#", "internet.mundo-r.com", "", ""},
    {"2143", "*99#", "CARREFOURINTERNET", "", ""},
    {"2144", "*99#", "internet", "", ""},
    {"2145", "*99#", "movistar.es", "movistar", "movistar"},
    {"2145", "*99#", "tuenti.com", "tuenti", "tuenti"},
    {"2146", "*99#", "ac.vodafone.es", "vodafone", "vodafone"},
    {"2146", "*99#", "lowi.private.omv.es", "", ""},
    {"2146", "*99#", "gprsmov.pepephone.com", "", ""},
    {"2147", "*99#", "movistar.es", "movistar", "movistar"},
    {"2148", "*99#", "internet.euskaltel.mobi", "CLIENTE", "EUSKALTEL"},
    {"2148", "*99#", "internet.racc.net", "CLIENTERACC", "RACC"},
    {"2149", "*99#", "internet", "", ""},
    {"21416", "*99#", "internet.telecable.es", "telecable", "telecable"},
    {"21418", "*99#", "internet.ono.com", "", ""},
    {"21419", "*99#", "gprs-service.com", "", ""},
    {"21421", "*99#", "jazzinternet", "", ""},
    {"21424", "*99#", "gprs.eroskimovil.es", "wap@wap", "wap125"},
    {"21630", "*99#", "internet", "", ""},
    {"21670", "*99#", "internet.vodafone.net", "", ""},
    {"21601", "*99#", "net", "", ""},
    {"2185", "*99#", "mobisgprs1", "", ""},
    {"21910", "*99#", "gprs0.vipnet.hr", "", ""},
    {"22003", "*99#", "wifigsp", "mts", "64"},
    {"2221", "*99#", "ibox.tim.it", "", ""},
    {"2221", "*99#", "ibox.tim.it", "", ""},
    {"22210", "*99#", "web.omnitel.it", "", ""},
    {"22288", "*99#", "internet.wind", "", ""},
    {"22298", "*99#", "INTERNET", "", ""},
    {"22299", "*99#", "tre.it", "tre", "tre"},
    {"22299", "*99#", "apn.fastweb.it", "", ""},
    {"2281", "*99#", "corporate.swisscom.ch", "testprofil", "temporary"},
    {"2281", "*99#", "gprs.swisscom.ch gprs", "gprs", "Swisscom"},
    {"2282", "*99#", "internet", "", ""},
    {"2283", "*99#", "internet", "", ""},
    {"2283", "*99#", "click", "", ""},
    {"2283", "*99#", "mobileoffice3g", "", ""},
    {"2301", "*99#", "internet.t-mobile.cz", "", ""},
    {"2302", "*99#", "internet", "", ""},
    {"2303", "*99#", "internet", "", ""},
    {"2304", "*99#", "internet", "", ""},
    {"2313", "*99#", "internet", "", ""},
    {"2321", "*99#", "A1.net", "ppp@a1plus.at", "ppp"},
    {"2323", "*99#", "internet.t-mobile.at", "t-mobile", "tm"},
    {"2323", "*99#", "business.gprsinternet", "t-mobile", "tm"},
    {"2325", "*99#", "orange.web", "web", "web"},
    {"2327", "*99#", "web", "web@telering.at", "web"},
    {"2327", "*99#", "webaut", "", ""},
    {"2327", "*99#", "webapn.at", "", ""},
    {"23210", "*99#", "drei.at", "drei", ""},
    {"23211", "*99#", "bob.at", "data@bob.at", "ppp"},
    {"23212", "*99#", "web.yesss.at", "web", ""},
    {"23213", "*99#", "internet.at.upcmobile.com", "", ""},
    {"23410", "*99#", "mobile.o2.co.uk", "o2web", "password"},
    {"23410", "*99#", "prepay.tesco-mobile.com", "tescowap", "password"},
    {"23415", "*99#", "Internet", "web", "web"},
    {"23415", "*99#", "uk.lebara.mobi", "wap", "wap"},
    {"23415", "*99#", "wap.vodafone.co.uk", "wap", "wap"},
    {"23415", "*99#", "payg.talkmobile.co.uk", "wap", "wap"},
    {"23415", "*99#", "talkmobile.co.uk", "wap", "wap"},
    {"23420", "*99#", "three.co.uk", "", ""},
    {"23420", "*99#", "id", "", ""},
    {"23420", "*99#", "superdrug.net", "", ""},
    {"23426", "*99#", "data.lycamobile.co.uk", "lmpl", "plus"},
    {"23430", "*99#", "everywhere", "eesecure", "secure"},
    {"23430", "*99#", "general.t-mobile.uk", "user", "wap"},
    {"23431", "*99#", "goto.virginmobile.uk", "user", ""},
    {"23432", "*99#", "goto.virginmobile.uk", "user", ""},
    {"23433", "*99#", "orangeinternet", "", ""},
    {"23450", "*99#", "pepper", "", ""},
    {"23450", "*99#", "internet.cheeriot.com", "", ""},
    {"23450", "*99#", "internet.iotpet.net", "", ""},
    {"23450", "*99#", "internet.simfony.net", "", ""},
    {"23457", "*99#", "mobile.sky", "", ""},
    {"23458", "*99#", "web.manxpronto.net", "gprs", "gprs"},
    {"2381", "*99#", "internet", "", ""},
    {"2382", "*99#", "internet", "", ""},
    {"2382", "*99#", "internet", "", ""},
    {"2386", "*99#", "data.tre.dk", "", ""},
    {"2386", "*99#", "bredband.tre.dk", "", ""},
    {"23820", "*99#", "www.internet.mtelia.dk", "telia", "1010"},
    {"23820", "*99#", "www.internet.mtelia.dk", "", ""},
    {"23820", "*99#", "internet.ts.m2m", "", ""},
    {"23830", "*99#", "web.orange.dk", "", ""},
    {"23877", "*99#", "internet", "", ""},
    {"2401", "*99#", "online.telia.se", "", ""},
    {"2402", "*99#", "data.tre.se", "", ""},
    {"2402", "*99#", "bredband.tre.se", "", ""},
    {"2404", "*99#", "data.tre.se", "", ""},
    {"2404", "*99#", "bredband.tre.se", "", ""},
    {"2405", "*99#", "online.telia.se", "", ""},
    {"2405", "*99#", "mobileinternet.tele2.se", "", ""},
    {"2406", "*99#", "internet.telenor.se", "", ""},
    {"2408", "*99#", "internet.telenor.se", "", ""},
    {"24007", "*99#", "mobileinternet.tele2.se", "", ""},
    {"240077", "*99#", "data.comviq.se", "", ""},
    {"240078", "*99#", "data.comviq.se", "", ""},
    {"2421", "*99#", "Telenor", "", ""},
    {"2421", "*99#", "Telenor", "", ""},
    {"2422", "*99#", "internet.netcom.no", "", ""},
    {"2423", "*99#", "www.teletopia.no", "", ""},
    {"2425", "*99#", "internet.tele2.no", "wap", "wap"},
    {"2445", "*99#", "internet", "", ""},
    {"24412", "*99#", "internet", "", ""},
    {"24491", "*99#", "internet", "", ""},
    {"2461", "*99#", "gprs.omnitel.net", "", ""},
    {"2462", "*99#", "banga", "", ""},
    {"2463", "*99#", "internet.tele2.lt", "", ""},
    {"2471", "*99#", "internet.lmt.lv", "", ""},
    {"2472", "*99#", "internet.tele2.lv", "", ""},
    {"2481", "*99#", "internet.emt.ee", "", ""},
    {"2482", "*99#", "internet", "", ""},
    {"2483", "*99#", "internet.tele2.ee", "wap", "wap"},
    {"2551", "*99#", "www.umc.ua", "", ""},
    {"2553", "*99#", "www.kyivstar.net", "igprs", "internet"},
    {"2571", "*99#", "web.velcom.by", "web", "web"},
    {"26012", "*99#", "internet.cp", "", ""},
    {"26001", "*99#", "internet", "", ""},
    {"26002", "*99#", "internet", "", ""},
    {"26003", "*99#", "internet", "internet", "internet"},
    {"26006", "*99#", "internet", "", ""},
    {"2621", "*99#", "internet.telekom", "tm", "tm"},
    {"2621", "*99#", "internet.t-mobile", "t-mobile", "tm"},
    {"2622", "*99#", "web.vodafone.de", "", ""},
    {"2622", "*99#", "web.vodafone.de", "", ""},
    {"2622", "*99#", "web.vodafone.de", "", ""},
    {"2622", "*99#", "web.vodafone.de", "", ""},
    {"2622", "*99#", "web.vodafone.de", "", ""},
    {"2623", "*99#", "internet.eplus.de", "eplus", "internet"},
    {"2623", "*99#", "internet.eplus.de", "eplus", "eplus"},
    {"2623", "*99#", "internet.eplus.de", "simyo", "simyo"},
    {"2623", "*99#", "internet.eplus.de", "blau", "blau"},
    {"2623", "*99#", "tagesflat.eplus.de", "blau", "blau"},
    {"2623", "*99#", "internet.eplus.de", "eplus", "internet"},
    {"2623", "*99#", "tagesflat.eplus.de", "eplus", "internet"},
    {"2627", "*99#", "internet", "", ""},
    {"2627", "*99#", "internet.partner1", "", ""},
    {"2627", "*99#", "webmobil1", "", ""},
    {"2627", "*99#", "pinternet.interkom.de", "", ""},
    {"2627", "*99#", "pinternet.interkom.de", "", ""},
    {"2627", "*99#", "internet.partner", "", ""},
    {"2701", "*99#", "web.pt.lu", "", ""},
    {"27077", "*99#", "internet", "tango", "tango"},
    {"27099", "*99#", "orange.lu", "", ""},
    {"27099", "*99#", "vox.lu", "", ""},
    {"2721", "*99#", "isp.vodafone.ie", "vodafone", "vodafone"},
    {"2722", "*99#", "internet", "user", "user123"},
    {"2723", "*99#", "isp.mymeteor.ie", "my", "meteor"},
    {"2741", "*99#", "gprs.simi.is", "", ""},
    {"2781", "*99#", "internet", "", ""},
    {"27821", "*99#", "gosurfingun", "", ""},
    {"2801", "*99#", "cytamobile", "user", "pass"},
    {"28010", "*99#", "internet (or) wap", "wap", "wap"},
    {"28020", "*99#", "ip.primetel", "(blank)", "(blank)"},
    {"2841", "*99#", "inet-gprs.mtel.bg", "", ""},
    {"2843", "284", "internet.vivacom.bg", "VIVACOM", "VIVACOM"},
    {"2845", "*99#", "telenorbg", "", ""},
    {"2861", "*99#", "internet", "", ""},
    {"2862", "*99#", "telsim", "", ""},
    {"2863", "*99#", "internet", "", ""},
    {"29340", "*99#", "internet.simobil.si", "simobil", "internet"},
    {"29341", "*99#", "internet", "mobitel", "internet"},
    {"2941", "*99#", "internet", "internet", "mobimak"},
    {"2952", "*99#", "internet-ofl", "", ""},
    {"2955", "*99#", "FL1", "gprs@a1plus.at", "FL1"},
    {"30237", "*99#", "internet.fido.ca", "", ""},
    {"302370", "*99#", "internet.fido.ca", "", ""},
    {"302720", "*99#", "internet.com", "wapuser1", "wap"},
    {"31016", "*99#", "internet2.voicestream.com", "", ""},
    {"31021", "*99#", "internet2.voicestream.com", "", ""},
    {"31022", "*99#", "internet2.voicestream.com", "", ""},
    {"31023", "*99#", "internet2.voicestream.com", "", ""},
    {"31024", "*99#", "internet2.voicestream.com", "", ""},
    {"31025", "*99#", "internet2.voicestream.com", "", ""},
    {"31026", "*99#", "internet2.voicestream.com", "", ""},
    {"31027", "*99#", "internet2.voicestream.com", "", ""},
    {"31030", "*99#", "private.centennialwireless.com", "privuser", "priv"},
    {"31031", "*99#", "internet2.voicestream.com", "", ""},
    {"310150", "*99#", "ISP.CINGULAR", "ISP@CINGULARGPRS.COM", "CINGULAR1"},
    {"310160", "*99#", "internet2.voicestream.com", "", ""},
    {"310170", "*99#", "ISP.CINGULAR", "ISP@CINGULARGPRS.COM", "CINGULAR1"},
    {"310200", "*99#", "internet2.voicestream.com", "", ""},
    {"310210", "*99#", "internet2.voicestream.com", "", ""},
    {"310220", "*99#", "internet2.voicestream.com", "", ""},
    {"310230", "*99#", "internet2.voicestream.com", "", ""},
    {"310240", "*99#", "internet2.voicestream.com", "", ""},
    {"310250", "*99#", "internet2.voicestream.com", "", ""},
    {"310260", "*99#", "internet2.voicestream.com", "", ""},
    {"310270", "*99#", "internet2.voicestream.com", "", ""},
    {"310280", "*99#", "internet2.voicestream.com", "", ""},
    {"310290", "*99#", "internet2.voicestream.com", "", ""},
    {"310300", "*99#", "internet2.voicestream.com", "", ""},
    {"310310", "*99#", "internet2.voicestream.com", "", ""},
    {"310320", "*99#", "internet2.voicestream.com", "", ""},
    {"310380", "*99#", "ISP.CINGULAR", "ISP@CINGULARGPRS.COM", "CINGULAR1"},
    {"310410", "*99#", "ISP.CINGULAR", "ISP@CINGULARGPRS.COM", "CINGULAR1"},
    {"310420", "*99#", "web.gocbw.com", "cbw", ""},
    {"310660", "*99#", "internet2.voicestream.com", "", ""},
    {"310800", "*99#", "internet2.voicestream.com", "", ""},
    {"310980", "*99#", "ISP.CINGULAR", "ISP@CINGULARGPRS.COM", "CINGULAR1"},
    {"31130", "*99#", "internet.indigo", "user1@indigo.internet", "indigo"},
    {"311360", "*99#", "isp.stelera.net", "", ""},
    {"3136", "*9579#", "internet", "user", "user"},
    {"3145", "*9597#", "internet", "user", "user"},
    {"334020", "*99#", "internet.itelcel.com", "webgprs", "webgprs2003"},
    {"334020", "*99#", "ba.amx", "webgprs", "webgprs2002"},
    {"334030", "*99#", "internet.movistar.mx", "movistar", "movistar"},
    {"334050", "*99#", "modem.iusacellgsm.mx", "iusacellgsm", "iusacellgsm"},
    {"334090", "*99#", "modem.nexteldata.com.mx", "", ""},
    {"3385", "*99#", "web.digiceljamaica.com", "wapuser", "wap03jam"},
    {"338180", "*99#", "internet", "", ""},
    {"3401", "*99#", "orangewap", "", ""},
    {"34712", "*99#", "internet", "", ""},
    {"36291", "*99#", "utsnet.uts.an", "netuser", "net123"},
    {"3631", "*99#", "internet.setar.aw", "", ""},
    {"3701", "*99#", "orangenet.com.do", "", ""},
    {"3702", "*99#", "ba.amx", "", ""},
    {"37001", "*99#", "orangenet.com.do", "", ""},
    {"37002", "*99#", "ba.amx", "", ""},
    {"4012", "*99#", "internet", "", ""},
    {"40177", "*99#", "internet.tele2.kz", "", ""},
    {"40177", "*99#", "mms", "", ""},
    {"40177", "*99#", "internet.altel.kz", "", ""},
    {"40107", "*99#", "internet.altel.kz", "", ""},
    {"40101", "*99#", "internet.beeline.kz", "", ""},
    {"4041", "*99#", "www", "", ""},
    {"4042", "*99#", "airtelgprs.com", "", ""},
    {"4043", "*99#", "airtelgprs.com", "", ""},
    {"4044", "*99#", "internet", "", ""},
    {"4045", "*99#", "www", "", ""},
    {"4047", "*99#", "internet", "", ""},
    {"4049", "*99#", "smartnet", "", ""},
    {"40410", "*99#", "airtelgprs.com", "", ""},
    {"40411", "*99#", "www", "", ""},
    {"40412", "*99#", "internet", "", ""},
    {"40413", "*99#", "www", "", ""},
    {"40414", "*99#", "spicegprs", "", ""},
    {"40415", "*99#", "www", "", ""},
    {"40417", "*99#", "aircelgprs", "", ""},
    {"40419", "*99#", "internet", "", ""},
    {"40420", "*99#", "www", "", ""},
    {"40421", "*99#", "bplgprs.com", "", ""},
    {"40422", "*99#", "internet", "", ""},
    {"40424", "*99#", "internet", "", ""},
    {"40425", "*99#", "aircelgprs", "", ""},
    {"40427", "*99#", "www", "", ""},
    {"40427", "*99#", "bplgprs.com", "", ""},
    {"40429", "*99#", "aircelgprs", "", ""},
    {"40430", "*99#", "www", "", ""},
    {"40431", "*99#", "airtelgprs.com", "", ""},
    {"40434", "*99#", "bsnlnet", "", ""},
    {"40436", "*99#", "smartnet", "", ""},
    {"40437", "*99#", "aircelgprs", "", ""},
    {"40438", "*99#", "bsnlnet", "", ""},
    {"40440", "*99#", "airtelgprs.com", "", ""},
    {"40441", "*99#", "aircelgprs", "", ""},
    {"40442", "*99#", "aircelgprs", "", ""},
    {"40444", "*99#", "internet", "", ""},
    {"40445", "*99#", "airtelgprs.com", "", ""},
    {"40449", "*99#", "airtelgprs.com", "", ""},
    {"40451", "*99#", "bsnlnet", "", ""},
    {"40453", "*99#", "bsnlnet", "", ""},
    {"40454", "*99#", "bsnlnet", "", ""},
    {"40455", "*99#", "bsnlnet", "", ""},
    {"40456", "*99#", "internet", "", ""},
    {"40457", "*99#", "bsnlnet", "", ""},
    {"40458", "*99#", "bsnlnet", "", ""},
    {"40459", "*99#", "bsnlnet", "", ""},
    {"40460", "*99#", "www", "", ""},
    {"40462", "*99#", "bsnlnet", "", ""},
    {"40464", "*99#", "bsnlnet", "", ""},
    {"40466", "*99#", "bsnlnet", "", ""},
    {"40467", "*99#", "smartnet", "", ""},
    {"40468", "*99#", "ppshsdpa", "mtnl", "mtnl123"},
    {"40468", "*99#", "mtnl.net", "", ""},
    {"40469", "*99#", "pps3g", "mtnl", "mtnl123"},
    {"40469", "*99#", "mtnl.net", "", ""},
    {"40470", "*99#", "airtelgprs.com", "", ""},
    {"40472", "*99#", "bsnlnet", "", ""},
    {"40474", "*99#", "bsnlnet", "", ""},
    {"40478", "*99#", "internet", "", ""},
    {"40481", "*99#", "bsnlnet", "", ""},
    {"40482", "*99#", "internet", "", ""},
    {"40483", "*99#", "smartnet", "", ""},
    {"40484", "*99#", "www", "", ""},
    {"40485", "*99#", "smartnet", "", ""},
    {"40486", "*99#", "www", "", ""},
    {"40487", "*99#", "internet", "", ""},
    {"40488", "*99#", "www", "", ""},
    {"40489", "*99#", "internet", "", ""},
    {"40490", "*99#", "airtelgprs.com", "", ""},
    {"40491", "*99#", "aircelgprs", "", ""},
    {"40492", "*99#", "airtelgprs.com", "", ""},
    {"40493", "*99#", "airtelgprs.com", "", ""},
    {"40495", "*99#", "airtelgprs.com", "", ""},
    {"40496", "*99#", "airtelgprs.com", "", ""},
    {"40497", "*99#", "airtelgprs.com", "", ""},
    {"40498", "*99#", "airtelgprs.com", "", ""},
    {"404927", "*99#", "uninor", "", ""},
    {"40401", "*99#", "www", "", ""},
    {"40402", "*99#", "airtelgprs.com", "", ""},
    {"40403", "*99#", "airtelgprs.com", "", ""},
    {"40404", "*99#", "internet", "", ""},
    {"40405", "*99#", "www", "", ""},
    {"40407", "*99#", "internet", "", ""},
    {"40409", "*99#", "smartnet", "", ""},
    {"40410", "*99#", "airtelgprs.com", "", ""},
    {"40411", "*99#", "www", "", ""},
    {"40412", "*99#", "internet", "", ""},
    {"40413", "*99#", "www", "", ""},
    {"40415", "*99#", "www", "", ""},
    {"40416", "*99#", "airtelgprs.com", "", ""},
    {"40417", "*99#", "aircelinternet", "", ""},
    {"40418", "*99#", "smartnet", "", ""},
    {"40419", "*99#", "internet", "", ""},
    {"40420", "*99#", "www", "", ""},
    {"40422", "*99#", "internet", "", ""},
    {"40424", "*99#", "internet", "", ""},
    {"40425", "*99#", "aircelinternet", "", ""},
    {"40427", "*99#", "www", "", ""},
    {"40428", "*99#", "aircelinternet", "", ""},
    {"40429", "*99#", "aircelinternet", "", ""},
    {"40430", "*99#", "www", "", ""},
    {"40431", "*99#", "airtelgprs.com", "", ""},
    {"40433", "*99#", "aircelinternet", "", ""},
    {"40434", "*99#", "bsnlnet", "", ""},
    {"40435", "*99#", "aircelinternet", "", ""},
    {"40436", "*99#", "smartnet", "", ""},
    {"40437", "*99#", "aircelinternet", "", ""},
    {"40438", "*99#", "bsnlnet", "", ""},
    {"40440", "*99#", "airtelgprs.com", "", ""},
    {"40441", "*99#", "aircelinternet", "", ""},
    {"40442", "*99#", "aircelinternet", "", ""},
    {"40443", "*99#", "www", "", ""},
    {"40445", "*99#", "airtelgprs.com", "", ""},
    {"40446", "*99#", "www", "", ""},
    {"40449", "*99#", "airtelgprs.com", "", ""},
    {"40450", "*99#", "smartnet", "", ""},
    {"40451", "*99#", "bsnlnet", "", ""},
    {"40452", "*99#", "smartnet", "", ""},
    {"40453", "*99#", "bsnlnet", "", ""},
    {"40454", "*99#", "bsnlnet", "", ""},
    {"40455", "*99#", "bsnlnet", "", ""},
    {"40456", "*99#", "internet", "", ""},
    {"40457", "*99#", "bsnlnet", "", ""},
    {"40458", "*99#", "bsnlnet", "", ""},
    {"40459", "*99#", "bsnlnet", "", ""},
    {"40460", "*99#", "www", "", ""},
    {"40462", "*99#", "bsnlnet", "", ""},
    {"40464", "*99#", "bsnlnet", "", ""},
    {"40466", "*99#", "bsnlnet", "", ""},
    {"40467", "*99#", "smartnet", "", ""},
    {"40470", "*99#", "airtelgprs.com", "", ""},
    {"40471", "*99#", "bsnlnet", "", ""},
    {"40472", "*99#", "bsnlnet", "", ""},
    {"40473", "*99#", "bsnlnet", "", ""},
    {"40474", "*99#", "bsnlnet", "", ""},
    {"40475", "*99#", "bsnlnet", "", ""},
    {"40476", "*99#", "bsnlnet", "", ""},
    {"40477", "*99#", "bsnlnet", "", ""},
    {"40478", "*99#", "internet", "", ""},
    {"40479", "*99#", "bsnlnet", "", ""},
    {"40480", "*99#", "bsnlnet", "", ""},
    {"40481", "*99#", "bsnlnet", "", ""},
    {"40482", "*99#", "internet", "", ""},
    {"40483", "*99#", "smartnet", "", ""},
    {"40484", "*99#", "www", "", ""},
    {"40485", "*99#", "smartnet", "", ""},
    {"40486", "*99#", "www", "", ""},
    {"40487", "*99#", "internet", "", ""},
    {"40488", "*99#", "www", "", ""},
    {"40489", "*99#", "internet", "", ""},
    {"40490", "*99#", "airtelgprs.com", "", ""},
    {"40491", "*99#", "aircelinternet", "", ""},
    {"40492", "*99#", "airtelgprs.com", "", ""},
    {"40493", "*99#", "airtelgprs.com", "", ""},
    {"40494", "*99#", "airtelgprs.com", "", ""},
    {"40495", "*99#", "airtelgprs.com", "", ""},
    {"40496", "*99#", "airtelgprs.com", "", ""},
    {"40497", "*99#", "airtelgprs.com", "", ""},
    {"40498", "*99#", "airtelgprs.com", "", ""},
    {"4051", "*99#", "smartnet", "", ""},
    {"4053", "*99#", "smartnet", "", ""},
    {"4054", "*99#", "smartnet", "", ""},
    {"4055", "*99#", "smartnet", "", ""},
    {"40510", "*99#", "smartnet", "", ""},
    {"40513", "*99#", "smartnet", "", ""},
    {"40525", "*99#", "tata.docomo.internet", "", ""},
    {"40525", "*99#", "tatadocomo3g", "", ""},
    {"40526", "*99#", "tata.docomo.internet", "", ""},
    {"40526", "*99#", "tatadocomo3g", "", ""},
    {"40527", "*99#", "tata.docomo.internet", "", ""},
    {"40527", "*99#", "tatadocomo3g", "", ""},
    {"40528", "*99#", "tatadocomo3g", "", ""},
    {"40529", "*99#", "tata.docomo.internet", "", ""},
    {"40529", "*99#", "tatadocomo3g", "", ""},
    {"40530", "*99#", "tata.docomo.internet", "", ""},
    {"40530", "*99#", "tatadocomo3g", "", ""},
    {"40531", "*99#", "tata.docomo.internet", "", ""},
    {"40531", "*99#", "tatadocomo3g", "", ""},
    {"40532", "*99#", "tata.docomo.internet", "", ""},
    {"40532", "*99#", "tatadocomo3g", "", ""},
    {"40533", "*99#", "tata.docomo.internet", "", ""},
    {"40533", "*99#", "tatadocomo3g", "", ""},
    {"40534", "*99#", "tata.docomo.internet", "", ""},
    {"40534", "*99#", "tatadocomo3g", "", ""},
    {"40535", "*99#", "tata.docomo.internet", "", ""},
    {"40535", "*99#", "tatadocomo3g", "", ""},
    {"40536", "*99#", "tata.docomo.internet", "", ""},
    {"40536", "*99#", "tatadocomo3g", "", ""},
    {"40537", "*99#", "tata.docomo.internet", "", ""},
    {"40537", "*99#", "tatadocomo3g", "", ""},
    {"40538", "*99#", "tata.docomo.internet", "", ""},
    {"40538", "*99#", "tatadocomo3g", "", ""},
    {"40539", "*99#", "tata.docomo.internet", "", ""},
    {"40539", "*99#", "tatadocomo3g", "", ""},
    {"40540", "*99#", "tatadocomo3g", "", ""},
    {"40541", "*99#", "tatadocomo3g", "", ""},
    {"40542", "*99#", "tata.docomo.internet", "", ""},
    {"40542", "*99#", "tatadocomo3g", "", ""},
    {"40543", "*99#", "tata.docomo.internet", "", ""},
    {"40543", "*99#", "tatadocomo3g", "", ""},
    {"40544", "*99#", "tata.docomo.internet", "", ""},
    {"40544", "*99#", "tatadocomo3g", "", ""},
    {"40545", "*99#", "tata.docomo.internet", "", ""},
    {"40545", "*99#", "tatadocomo3g", "", ""},
    {"40546", "*99#", "tata.docomo.internet", "", ""},
    {"40546", "*99#", "tatadocomo3g", "", ""},
    {"40547", "*99#", "tata.docomo.internet", "", ""},
    {"40547", "*99#", "tatadocomo3g", "", ""},
    {"40551", "*99#", "airtelgprs.com", "", ""},
    {"40552", "*99#", "airtelgprs.com", "", ""},
    {"40554", "*99#", "airtelgprs.com", "", ""},
    {"40556", "*99#", "airtelgprs.com", "", ""},
    {"40566", "*99#", "www", "", ""},
    {"40570", "*99#", "internet", "", ""},
    {"405750", "*99#", "www", "", ""},
    {"405751", "*99#", "www", "", ""},
    {"405752", "*99#", "www", "", ""},
    {"405754", "*99#", "www", "", ""},
    {"405756", "*99#", "www", "", ""},
    {"405799", "*99#", "internet", "", ""},
    {"405800", "*99#", "aircelgprs", "", ""},
    {"405801", "*99#", "aircelgprs", "", ""},
    {"405802", "*99#", "aircelgprs", "", ""},
    {"405803", "*99#", "aircelgprs", "", ""},
    {"405804", "*99#", "aircelgprs", "", ""},
    {"405805", "*99#", "aircelgprs", "", ""},
    {"405806", "*99#", "aircelgprs", "", ""},
    {"405807", "*99#", "aircelgprs", "", ""},
    {"405808", "*99#", "aircelgprs", "", ""},
    {"405809", "*99#", "aircelgprs", "", ""},
    {"405810", "*99#", "aircelgprs", "", ""},
    {"405811", "*99#", "aircelgprs", "", ""},
    {"405812", "*99#", "aircelgprs", "", ""},
    {"405818", "*99#", "uninor", "", ""},
    {"405819", "*99#", "uninor", "", ""},
    {"405820", "*99#", "uninor", "", ""},
    {"405821", "*99#", "uninor", "", ""},
    {"405822", "*99#", "uninor", "", ""},
    {"405824", "*99#", "videocon", "", ""},
    {"405827", "*99#", "videocon", "", ""},
    {"405834", "*99#", "videocon", "", ""},
    {"405844", "*99#", "uninor", "", ""},
    {"405845", "*99#", "internet", "", ""},
    {"405848", "*99#", "internet", "", ""},
    {"405855", "*99#", "bplgprs.com", "", ""},
    {"405864", "*99#", "bplgprs.com", "", ""},
    {"405865", "*99#", "bplgprs.com", "", ""},
    {"405875", "*99#", "uninor", "", ""},
    {"405880", "*99#", "uninor", "", ""},
    {"405929", "*99#", "uninor", "", ""},
    {"40501", "*99#", "rcomnet", "", ""},
    {"40503", "*99#", "rcomnet", "", ""},
    {"40504", "*99#", "rcomnet", "", ""},
    {"40505", "*99#", "rcomnet", "", ""},
    {"40506", "*99#", "rcomnet", "", ""},
    {"40507", "*99#", "rcomnet", "", ""},
    {"40508", "*99#", "rcomnet", "", ""},
    {"40509", "*99#", "rcomnet", "", ""},
    {"40510", "*99#", "rcomnet", "", ""},
    {"40511", "*99#", "rcomnet", "", ""},
    {"40512", "*99#", "rcomnet", "", ""},
    {"40513", "*99#", "rcomnet", "", ""},
    {"40514", "*99#", "rcomnet", "", ""},
    {"40515", "*99#", "rcomnet", "", ""},
    {"40517", "*99#", "rcomnet", "", ""},
    {"40518", "*99#", "rcomnet", "", ""},
    {"40519", "*99#", "rcomnet", "", ""},
    {"40520", "*99#", "rcomnet", "", ""},
    {"40521", "*99#", "rcomnet", "", ""},
    {"40522", "*99#", "rcomnet", "", ""},
    {"40523", "*99#", "rcomnet", "", ""},
    {"40551", "*99#", "airtelgprs.com", "", ""},
    {"40552", "*99#", "airtelgprs.com", "", ""},
    {"40553", "*99#", "airtelgprs.com", "", ""},
    {"40554", "*99#", "airtelgprs.com", "", ""},
    {"40555", "*99#", "airtelgprs.com", "", ""},
    {"40556", "*99#", "airtelgprs.com", "", ""},
    {"40566", "*99#", "www", "", ""},
    {"40567", "*99#", "www", "", ""},
    {"40570", "*99#", "internet", "", ""},
    {"405750", "*99#", "www", "", ""},
    {"405751", "*99#", "www", "", ""},
    {"405752", "*99#", "www", "", ""},
    {"405753", "*99#", "www", "", ""},
    {"405754", "*99#", "www", "", ""},
    {"405755", "*99#", "www", "", ""},
    {"405756", "*99#", "www", "", ""},
    {"405799", "*99#", "interne", "", ""},
    {"405800", "*99#", "aircelinternet", "", ""},
    {"405801", "*99#", "aircelinternet", "", ""},
    {"405802", "*99#", "aircelinternet", "", ""},
    {"405803", "*99#", "aircelinternet", "", ""},
    {"405804", "*99#", "aircelinternet", "", ""},
    {"405805", "*99#", "aircelinternet", "", ""},
    {"405806", "*99#", "aircelinternet", "", ""},
    {"405807", "*99#", "aircelinternet", "", ""},
    {"405808", "*99#", "aircelinternet", "", ""},
    {"405809", "*99#", "aircelinternet", "", ""},
    {"405810", "*99#", "aircelinternet", "", ""},
    {"405811", "*99#", "aircelinternet", "", ""},
    {"405812", "*99#", "aircelinternet", "", ""},
    {"405845", "*99#", "internet", "", ""},
    {"405846", "*99#", "internet", "", ""},
    {"405847", "*99#", "internet", "", ""},
    {"405848", "*99#", "internet", "", ""},
    {"405849", "*99#", "internet", "", ""},
    {"405850", "*99#", "internet", "", ""},
    {"405851", "*99#", "internet", "", ""},
    {"405852", "*99#", "internet", "", ""},
    {"405853", "*99#", "internet", "", ""},
    {"405854", "*99#", "WWW", "", ""},
    {"405855", "*99#", "WWW", "", ""},
    {"405856", "*99#", "WWW", "", ""},
    {"405857", "*99#", "WWW", "", ""},
    {"405858", "*99#", "WWW", "", ""},
    {"405859", "*99#", "WWW", "", ""},
    {"405860", "*99#", "WWW", "", ""},
    {"405861", "*99#", "WWW", "", ""},
    {"405862", "*99#", "WWW", "", ""},
    {"405863", "*99#", "WWW", "", ""},
    {"405864", "*99#", "WWW", "", ""},
    {"405865", "*99#", "WWW", "", ""},
    {"405866", "*99#", "WWW", "", ""},
    {"405867", "*99#", "WWW", "", ""},
    {"405868", "*99#", "WWW", "", ""},
    {"405869", "*99#", "WWW", "", ""},
    {"405870", "*99#", "WWW", "", ""},
    {"405871", "*99#", "WWW", "", ""},
    {"405872", "*99#", "WWW", "", ""},
    {"405873", "*99#", "WWW", "", ""},
    {"405854", "", "jionet", "", ""},
    {"405855", "", "jionet", "", ""},
    {"405856", "", "jionet", "", ""},
    {"405872", "", "jionet", "", ""},
    {"405857", "", "jionet", "", ""},
    {"405858", "", "jionet", "", ""},
    {"405859", "", "jionet", "", ""},
    {"405860", "", "jionet", "", ""},
    {"405861", "", "jionet", "", ""},
    {"405862", "", "jionet", "", ""},
    {"405873", "", "jionet", "", ""},
    {"405863", "", "jionet", "", ""},
    {"405864", "", "jionet", "", ""},
    {"405874", "", "jionet", "", ""},
    {"405865", "", "jionet", "", ""},
    {"405866", "", "jionet", "", ""},
    {"405867", "", "jionet", "", ""},
    {"405868", "", "jionet", "", ""},
    {"405869", "", "jionet", "", ""},
    {"405871", "", "jionet", "", ""},
    {"405870", "", "jionet", "", ""},
    {"405840", "", "jionet", "", ""},
    {"4101", "*99#", "connect.mobilinkworld.com", "", ""},
    {"4101", "*99#", "Jazzconnect.mobilinkworld.com", "", ""},
    {"4102", "*99#", "eagle.com", "vwireless@eagle.com", "ptcl"},
    {"4103", "*99#", "ufone.pinternet", "ufone", "ufone"},
    {"4103", "*99#", "ufone.internet", "ufone", "ufone"},
    {"4104", "*99#", "zonginternet", "", ""},
    {"4106", "*99#", "internet", "Telenor", "Telenor"},
    {"4107", "*99#", "wap.warid", "", ""},
    {"4131", "*99#", "mobitel", "", ""},
    {"413002", "*99#", "dialogbb", "", ""},
    {"4138", "*99#", "hutch3g", "", ""},
    {"413003", "*99#", "ebb", "", ""},
    {"413005", "*99#", "airtellive", "", ""},
    {"4141", "*95#", "mptnet", "mpt", "user"},
    {"4145", "*9531#", "#777", "mectel@c800.mm", "123"},
    {"4151", "*99#", "usb.mic1.com.lb", "", ""},
    {"4153", "*99#", "touch", "", ""},
    {"4155", "*99#", "Ogero", "", ""},
    {"4161", "*99#", "Zain", "", ""},
    {"41677", "*99#", "net.mobilecom.jo", "internet", "internet"},
    {"4171", "*99#", "net.syriatel.com", "", ""},
    {"4172", "*99#", "internet", "", ""},
    {"4185", "*99#", "net.asiacell.com", "", ""},
    {"4192", "*99#", "pps", "", ""},
    {"4192", "*99#", "hpps", "", ""},
    {"4193", "*99#", "action.wataniya.com", "", ""},
    {"4194", "*99#", "viva", "", ""},
    {"4201", "*99#", "jawalnet.com.sa", "", ""},
    {"4201", "*99#", "Afaqwireless.com", "", ""},
    {"4203", "*99#", "Web1", "", ""},
    {"4203", "*99#", "web2", "", ""},
    {"4204", "*99#", "zain", "", ""},
    {"4205", "*99#", "internet", "", ""},
    {"4206", "*99#", "lebara", "", ""},
    {"4207", "*99#", "wogstc.com", "stc", "stc"},
    {"4222", "*99#", "taif", "taif", "taif"},
    {"4223", "*99#", "isp.nawras.com.om", "", ""},
    {"4242", "*99#", "etisalat.ae", "", ""},
    {"4243", "*99#", "du", "", ""},
    {"4251", "*99#", "uinternet", "", ""},
    {"4252", "*99#", "internetg", "", ""},
    {"4253", "*99#", "internet.pelephone.net.il", "pcl@3g", "pcl"},
    {"4261", "*99#", "batelco.com", "", ""},
    {"4262", "*99#", "internet", "", ""},
    {"4264", "*99#", "viva.bh", "", ""},
    {"4272", "*99#", "web.vodafone.com.qa", "", ""},
    {"4271", "*99#", "data", "", ""},
    {"42901", "*99#", "ntnet", "", ""},
    {"42902", "*99#", "web", "", ""},
    {"42903", "*8#", "hellonepal", "", ""},
    {"42904", "", "smart", "", ""},
    {"43211", "*99#", "mcinet", "", ""},
    {"43220", "*99#", "RighTel", "", ""},
    {"43235", "*99#", "mtnirancell", "", ""},
    {"4400", "*99#", "emb.ne.jp", "em", "em"},
    {"4401", "*99***1", "", "", ""},
    {"4402", "*99***1", "", "", ""},
    {"4403", "*99***1", "", "", ""},
    {"4404", "*99#", "", "", ""},
    {"4406", "*99#", "", "", ""},
    {"4409", "*99***1", "", "", ""},
    {"44010", "*99***1", "", "", ""},
    {"44011", "*99***1", "", "", ""},
    {"44012", "*99***1", "", "", ""},
    {"44013", "*99***1", "", "", ""},
    {"44014", "*99***1", "", "", ""},
    {"44015", "*99***1", "", "", ""},
    {"44016", "*99***1", "", "", ""},
    {"44017", "*99***1", "", "", ""},
    {"44018", "*99***1", "", "", ""},
    {"44019", "*99***1", "", "", ""},
    {"44020", "*99#", "", "", ""},
    {"44021", "*99#", "", "", ""},
    {"44022", "*99***1", "", "", ""},
    {"44023", "*99***1", "", "", ""},
    {"44024", "*99***1", "", "", ""},
    {"44025", "*99***1", "", "", ""},
    {"44026", "*99***1", "", "", ""},
    {"44027", "*99***1", "", "", ""},
    {"44028", "*99***1", "", "", ""},
    {"44029", "*99***1", "", "", ""},
    {"44030", "*99***1", "", "", ""},
    {"44031", "*99***1", "", "", ""},
    {"44032", "*99***1", "", "", ""},
    {"44033", "*99***1", "", "", ""},
    {"44034", "*99***1", "", "", ""},
    {"44035", "*99***1", "", "", ""},
    {"44036", "*99***1", "", "", ""},
    {"44037", "*99***1", "", "", ""},
    {"44038", "*99***1", "", "", ""},
    {"44039", "*99***1", "", "", ""},
    {"44040", "*99#", "", "", ""},
    {"44041", "*99#", "", "", ""},
    {"44042", "*99#", "", "", ""},
    {"44043", "*99#", "", "", ""},
    {"44044", "*99#", "", "", ""},
    {"44045", "*99#", "", "", ""},
    {"44046", "*99#", "", "", ""},
    {"44047", "*99#", "", "", ""},
    {"44048", "*99#", "", "", ""},
    {"44049", "*99***1", "", "", ""},
    {"44058", "*99***1", "", "", ""},
    {"44060", "*99***1", "", "", ""},
    {"44061", "*99***1", "", "", ""},
    {"44062", "*99***1", "", "", ""},
    {"44063", "*99***1", "", "", ""},
    {"44064", "*99***1", "", "", ""},
    {"44065", "*99***1", "", "", ""},
    {"44066", "*99***1", "", "", ""},
    {"44067", "*99***1", "", "", ""},
    {"44068", "*99***1", "", "", ""},
    {"44069", "*99***1", "", "", ""},
    {"44087", "*99***1", "", "", ""},
    {"44090", "*99#", "", "", ""},
    {"44092", "*99#", "", "", ""},
    {"44093", "*99#", "", "", ""},
    {"44094", "*99#", "", "", ""},
    {"44095", "*99#", "", "", ""},
    {"44096", "*99#", "", "", ""},
    {"44097", "*99#", "", "", ""},
    {"44098", "*99#", "", "", ""},
    {"44099", "*99***1", "", "", ""},
    {"4521", "*99#", "m-wap", "mms", "mms"},
    {"4522", "*99#", "m3-world", "mms", "mms"},
    {"4524", "*99#", "v-internet", "", ""},
    {"4526", "*99#", "e-connect", "", ""},
    {"4540", "*99#", "hkcsl", "", ""},
    {"4540", "*99#", "cslp3", "", ""},
    {"4543", "*99#", "mobile.three.com.hk", "", ""},
    {"4543", "*99#", "imobile.three.com.hk", "", ""},
    {"4543", "*99#", "ipc.three.com.hk", "", ""},
    {"4543", "*99#", "yahoo.three.com.hk", "", ""},
    {"4543", "*99#", "3gnet", "", ""},
    {"4544", "*99#", "web-g.three.com.hk", "", ""},
    {"4546", "*99#", "SmarTone-Vodafone", "", ""},
    {"45410", "*99#", "internet", "", ""},
    {"45412", "*99#", "peoples.net", "", ""},
    {"45415", "*99#", "SmarTone-Vodafone", "", ""},
    {"45416", "*99#", "pccwdata", "", ""},
    {"45416", "*99#", "pccwdata", "", ""},
    {"45419", "*99#", "pccw", "", ""},
    {"45419", "*99#", "pccw", "", ""},
    {"4550", "*99#", "smartgprs", "", ""},
    {"4550", "*99#", "smartgprs", "", ""},
    {"4551", "*99#", "ctm-mobile", "", ""},
    {"4551", "*99#", "ctm-mobile", "", ""},
    {"4553", "*99#", "web.hutchisonmacau.com", "", ""},
    {"4553", "*99#", "web.hutchisonmacau.com", "", ""},
    {"4554", "*99#", "ctm-mobile", "", ""},
    {"4554", "*99#", "ctm-mobile", "", ""},
    {"4561", "*99#", "cellcard", "", ""},
    {"4568", "*99#", "metfone", "", ""},
    {"4564", "*99#", "qb", "", ""},
    {"4562", "*99#", "smart", "", ""},
    {"4572", "*99***1#", "etlnet", "", ""},
    {"4578", "*99#", "beelinenet", "", ""},
    {"4573", "*99#", "unitel3g", "", ""},
    {"4571", "*99#", "ltcnet", "", ""},
    {"4600", "*99#", "cmnet", "guest", "guest"},
    {"4601", "*99#", "3gnet", "", ""},
    //{"4601",	  "*99#",	   "3gwap", 						"", 							 ""},
    //{"4601",	  "*99#",	   "3gwap", 						"", 							 ""},
    {"4661", "*99#", "internet", "", ""},
    {"4665", "*99#", "internet", "", ""},
    {"46688", "*99#", "internet", "", ""},
    {"46689", "*99#", "vibo", "", ""},
    {"46690", "*99#", "internet", "", ""},
    {"46692", "*99#", "internet", "", ""},
    {"46697", "*99#", "internet", "", ""},
    {"4701", "*500*1#", "gpinternet", "", ""},
    {"4701", "*500*2*1#", "gpinternet", "", ""},
    {"4701", "*500*3*1#", "gpinternet", "", ""},
    {"4701", "*500*4*1#", "gpinternet", "", ""},
    {"4701", "*500*5*1#", "gpinternet", "", ""},
    {"4701", "*500*6*1#", "gpinternet", "", ""},
    {"4701", "*500*7*1#", "gpinternet", "", ""},
    {"4701", "*500*9*1#", "gpinternet", "", ""},
    {"4701", "*500*10*1#", "gpinternet", "", ""},
    {"4701", "*500*11*1#", "gpinternet", "", ""},
    {"4702", "*140*7#", "INTERNET", "", ""},
    {"4703", "*222*1*1#", "blweb", "", ""},
    {"4703", "*222*1*9#", "blweb", "", ""},
    {"4703", "*222*1*13#", "blweb", "", ""},
    {"4703", "*222*1*14#", "blweb", "", ""},
    {"4703", "*222*1*8#", "blweb", "", ""},
    {"4704", "*99#", "gprsunl", "", ""},
    {"4704", "*99#", "WAP", "", ""},
    {"4705", "#777", "internet", "", ""},
    {"4707", "*99#", "Internet", "", ""},
    {"50212", "*99#", "unet", "maxis", "wap"},
    {"50213", "*99#", "internet", "", ""},
    {"50216", "*99#", "diginet", "", ""},
    {"50218", "*99#", "my3g", "", ""},
    {"50219", "*99#", "celcom3g", "", ""},
    {"502150", "*99#", "tunetalk", "", ""},
    {"502156", "", "altel", "", ""},
    {"50219", "", "xox3g", "", ""},
    {"50212", "*99#", "unet", "maxis", "wap"},
    {"50216", "*99#", "diginet", "", ""},
    {"50218", "*99#", "my3g", "", ""},
    {"50219", "*99#", "celcom3g", "", ""},
    {"5051", "*99#", "telstra.internet", "", ""},
    {"5052", "*99#", "internet", "", ""},
    {"5053", "*99#", "live.vodafone.com", "", ""},
    {"5056", "*99#", "3netaccess", "", ""},
    {"50514", "*99#", "vfinternet.au", "", ""},
    {"5052", "*99#", "Internet", "", ""},
    {"5051", "*99#", "telstra.bigpond", "", ""},
    {"5052", "*99#", "splns888a1", "", ""},
    {"50538", "*99#", "purtona.net", "", ""},
    {"5052", "*99#", "DODOLNS1", "", ""},
    {"5052", "*99#", "WirelessBroadband", "", ""},
    {"5052", "*99#", "CONNECT", "", ""},
    {"5052", "*99#", "connect", "", ""},
    {"5052", "*99#", "internet", "", ""},
    {"5052", "*99#", "primuslns1", "", ""},
    {"5052", "*99#", "internet", "", ""},
    {"5052", "*99#", "VirginBroadband", "", ""},
    {"5101", "*99#", "indosatgprs", "indosat", ""},
    {"5101", "*99#", "indosatm2", "", ""},
    {"5109", "", "smartfren4g", "smartfren", "smartfren"},
    {"51010", "*99#", "internet", "", ""},
    {"51011", "*99#", "internet", "", ""},
    {"51021", "*99#", "indosatgprs", "indosat", "indosat"},
    {"51089", "*99#", "3data", "3data", "3data"},
    {"51028", "#777", "smartfren", "smartfren", "smartfren"},
    {"51028", "#777", "smartfren4g", "smartfren4g", "smartfren4g"},
    {"51021", "*99#", "indosatgprs", "indosat", "indosat"},
    {"5108", "*99#", "internet", "", ""},
    {"51088", "", "internet", "", ""},
    {"51011", "*99#", "www.xlgprs.net", "", ""},
    {"5151", "*99#", "minternet", "", ""},
    {"5152", "*99#", "internet.globe.com.ph", "", ""},
    {"5153", "*99#", "SMARTBRO", "", ""},
    {"5151", "*99#", "fbband", "", ""},
    {"5151", "*99#", "minternet", "", ""},
    {"5152", "*99#", "internet.globe.com.ph", "", ""},
    {"5152", "*99#", "http.globe.com.ph", "", ""},
    {"5153", "*99#", "internet", "", ""},
    {"5153", "*99#", "SMARTBRO", "", ""},
    {"5201", "*99#", "internet", "", ""},
    {"52015", "*99#", "internet", "", ""},
    {"52015", "*99#", "internet", "", ""},
    {"52018", "*99#", "www.dtac.co.th", "", ""},
    {"52099", "*99***1#", "internet", "true", "true"},
    {"5205", "*99#", "www.dtac.co.th", "", ""},
    {"5200", "*99#", "internet", "true", "true"},
    {"5251", "*99#", "internet", "", ""},
    {"5252", "*99#", "internet", "", ""},
    {"5253", "*99#", "sunsurf", "", ""},
    {"5255", "*99#", "shwapint", "", ""},
    {"52811", "*99#", "dst.internet", "internet", "internet"},
    {"52811", "*99#", "dst.internet", "", ""},
    {"5301", "*99#", "www.vodafone.net.nz", "", ""},
    {"5301", "*99#", "www.m2.net.nz", "", ""},
    {"5301", "*99#", "www.orcon.net.nz", "", ""},
    {"5301", "*99#", "www.vodafone.net.nz", "", ""},
    {"5305", "*99#", "internet.telecom.co.nz", "", ""},
    {"53024", "*99#", "internet", "", ""},
    {"5305", "*99#", "www.orcon.net.nz", "", ""},
    {"5305", "*99#", "www.callplus.net.nz", "", ""},
    {"5305", "*99#", "internet.telecom.co.nz", "", ""},
    {"5305", "*99#", "internet.telecom.co.nz", "", ""},
    {"6021", "*99#", "mobinilweb", "", ""},
    {"6022", "*99#", "internet.vodafone.net", "internet", "internet"},
    {"6023", "*99#", "interent.etisalat", "", ""},
    {"6024", "*99#", "internet.TE.eg", "", ""},
    {"6031", "*99#", "internet", "internet", "internet"},
    {"6032", "*99#", "djezzy.internet", "", ""},
    {"6033", "*99#", "internet", "internet", "internet"},
    {"6040", "*99#", "internet1.meditel.ma", "MEDINET", "MEDINET"},
    {"6040", "*99#", "internet.orange.ma", "", ""},
    {"6041", "*99#", "www.iamgprs1.ma", "", ""},
    {"6041", "*99#", "www.iamgprs2.ma", "", ""},
    {"6042", "#777", "", "wana", "wana"},
    {"6051", "*99#", "weborange", "", ""},
    {"6052", "*99#", "internet.tn", "internet@TT1", "dim@net"},
    {"6053", "*99#", "internet.ooredoo.tn", "", ""},
    {"6053", "*99#", "internet.tunisiana.com", "internet", "internet"},
    {"6072", "*99#", "africellnet", "", ""},
    {"6082", "*99#", "web.sentel.com", "", ""},
    {"6101", "*99#", "web.malitel3.ml", "", ""},
    {"6187", "*99#", "web.cellcomnet.net", "", ""},
    {"6195", "*99#", "africell.sl", "", ""},
    {"6201", "*99#", "internet", "", ""},
    {"6202", "*99#", "Browse/internet", "", ""},
    {"6203", "*99#", "web.tigo.com.gh", "", ""},
    {"6203", "*99#", "web.tigo.com.gh", "", ""},
    {"6207", "*99#", "internet", "", ""},
    {"6206", "*99#", "internet", "", ""},
    {"62160", "*99#", "9mobile", "", ""},
    {"62150", "*99#", "glosecure", "secure", "secure"},
    {"6202", "*99#", "internet", "", ""},
    {"62130", "*99#", "web.gprs.mtnnigeria.net", "web", "web"},
    {"62150", "*99#", "APN", "Flat", "Flat"},
    {"63090", "*99#", "Internet", "", ""},
    {"6312", "*99#", "internet.unitel.co.ao", "", ""},
    {"6341", "*99#", "internet", "", ""},
    {"63510", "*99#", "internet.mtn", "", ""},
    {"63513", "*99#", "web.tigo.rw", "", ""},
    {"63514", "*99#", "internet.rw", "", ""},
    {"63517", "*99#", "internet4g.mango", "", ""},
    {"63517", "*99#", "internet4g.netlink", "", ""},
    {"6392", "*99#", "Safaricom", "saf", "data"},
    {"6393", "*99#", "internet", "", ""},
    {"6395", "*99#", "internet", "", ""},
    {"6397", "*99#", "bew.orange.co.ke", "", ""},
    {"63920", "#777", "", "orangefixedplus", "orange"},
    {"6402", "*99#", "tigoweb", "", ""},
    {"6403", "*99#", "znet", "", ""},
    {"6404", "*99#", "internet", "", ""},
    {"6405", "*99#", "internet", "", ""},
    {"6407", "*99#", "internet", "", ""},
    {"6409", "*99#", "internet", "", ""},
    {"6411", "*99#", "internet", "", ""},
    {"64110", "*99#", "internet", "", ""},
    {"64111", "*99#", "internet", "", ""},
    {"64114", "*99#", "internet", "", ""},
    {"64114", "*99#", "orange.ug", "", ""},
    {"64122", "*99#", "wap.waridtel.co.ug", "", ""},
    {"6431", "*99#", "Isp.mcel.mz", "", ""},
    {"6433", "*99#", "Internet", "", ""},
    {"6434", "*99#", "Internet", "", ""},
    {"6551", "*99#", "internet", "", ""},
    {"6551", "*99#", "Unrestricted", "", ""},
    {"6557", "*99#", "internet", "", ""},
    {"65510", "*99#", "internet", "mtnwap", "mtnwap"},
    {"65512", "*99#", "internet", "", ""},
    {"65510", "*99#", "myMTN", "", ""},
    {"6552", "*99#", "internet", "", ""},
    {"6552", "*99#", "TelkomInternet", "", ""},
    {"65512", "*99#", "afrihost", "", ""},
    {"65538", "*99#", "rain", "", ""},
    {"65512", "*99#", "axxess", "", ""},
    {"70401", "*99#", "Internet.ideasclaro", "", ""},
    {"70402", "*99#", "internet.tigo.gt", "", ""},
    {"70402", "*99#", "broadband.tigo.gt", "", ""},
    {"70402", "*99#", "wap.tigo.gt", "", ""},
    {"70403", "*99#", "internet.movistar.gt", "", ""},
    {"70404", "*99#", "ba.amx", "", ""},
    {"70602", "*99#", "web.digicelsv.com", "", ""},
    {"70603", "*99#", "broadband.tigo.sv", "", ""},
    {"70604", "*99#", "internet.movistar.sv", "movistarsv", "movistarsv"},
    {"70801", "*99#", "web.megatel.hn", "webmegatel", "webmegatel"},
    {"70802", "*99#", "broadband.tigo.hd", "", ""},
    {"710300", "*99#", "internet.movistar.ni", "movistarni", "movistarni"},
    {"710021", "*99#", "web.emovil", "webemovil", "webemovil"},
    {"71202", "*99#", "kolbi3g", "", ""},
    {"71401", "*99#", "apn01.cwpanama.com.pa", "", ""},
    {"71402", "*99#", "internet.movistar.pa", "movistarpa", "movistarpa"},
    {"71403", "*99#", "web.claro.com.pa", "claroweb", "claroweb"},
    {"71606", "*99#", "movistar.pe", "movistar@datos", "movistar"},
    {"71606", "*99#", "movistar.pe", "movistar@datos", "movistar"},
    {"71607", "*99#", "Modem.nextel.com.pe", "", ""},
    {"71610", "*99#", "claro.pe", "claro", "claro"},
    {"71610", "*99#", "claro.pe", "claro", "Claro Peru"},
    {"71615", "*99#", "bitel.pe", "", ""},
    {"71617", "*99#", "wap.nextel.com.pe", "", ""},
    {"71617", "*99#", "nextel.pe", "", ""},
    {"72410", "*99#", "zap.vivo.com.br", "vivo", "vivo"},
    {"72411", "*99#", "zap.vivo.com.br", "vivo", "vivo"},
    {"72415", "*99#", "sercomtel.com.br", "sercomtel", "sercomtel"},
    {"72416", "*99***1#", "brt.br", "brt", "brt"},
    {"72417", "*99#", "internet.br", "", ""},
    {"72423", "*99#", "zap.vivo.com.br", "vivo", "vivo"},
    {"72430", "*99#", "gprs.oi.com.br", "", ""},
    {"72431", "*99#", "gprs.oi.com.br", "", ""},
    {"72432", "*99#", "ctbc.br", "ctbc", "1212"},
    {"72433", "*99#", "ctbc.br", "ctbc", "1212"},
    {"72434", "*99#", "ctbc.br", "ctbc", "1212"},
    {"72438", "*99***1#", "claro.com.br", "claro", "claro"},
    {"72438", "*99***1#", "bandalarga.claro.com.br", "claro", "claro"},
    {"72439", "", "wap.nextel3g.net.br", "", ""},
    {"72400", "", "datacard.nextel3g.net.br", "", ""},
    {"72400", "", "wap.nextel3g.net.br", "", ""},
    {"72402", "*99#", "tim.br", "tim", "tim"},
    {"72403", "*99#", "tim.br", "tim", "tim"},
    {"72404", "*99#", "tim.br", "tim", "tim"},
    {"72405", "*99***1#", "claro.com.br", "claro", "claro"},
    {"72405", "*99***1#", "bandalarga.claro.com.br", "claro", "claro"},
    {"72406", "*99#", "zap.vivo.com.br", "vivo", "vivo"},
    {"72407", "*99#", "ctbc.br", "ctbc", "1212"},
    {"72408", "*99#", "tim.br", "tim", "tim"},
    {"72439", "", "datacard.nextel3g.net.br", "", ""},
    {"73010", "*99#", "imovil.entelpcs.cl", "entelpcs", "entelpcs"},
    {"73001", "*99#", "imovil.entelpcs.cl", "entelpcs", "entelpcs"},
    {"73002", "*99#", "web.tmovil.cl", "web", "web"},
    {"73002", "*99#", "wap.tmovil.cl", "wap", "wap"},
    {"73003", "*99#", "bam.clarochile.cl", "clarochile", "clarochile"},
    {"73003", "*99#", "bap.clarochile.cl", "clarochile", "clarochile"},
    {"73006", "*99#", "web.gtdmovil.cl", "webgtd", "webgtd"},
    {"73007", "*99#", "imovil.virginmobile.cl", "", ""},
    {"73008", "*99#", "movil.vtr.com", "vtrmovil", "vtrmovil"},
    {"73009", "*99#", "wap.nextelmovil.cl", "", ""},
    {"732130", "*99#", "lte.avantel.com.co", "", ""},
    {"732001", "*99#", "internet.movistar.com.co", "movistar", "movistar"},
    {"732020", "*99#", "une4glte.net.co", "une", "une"},
    {"732101", "*99#", "internet.claro.com.co", "CLAROWEB", "CLAROWEB"},
    {"732101", "*99#", "internet.comcel.com.co", "Comcelweb", "Comcelweb"},
    {"732102", "*99#", "une4glte.net.co", "une", "une"},
    {"732103", "*99#", "moviletb.net.co", "etb", "etb"},
    {"732103", "*99#", "movilexito.net.co", "", ""},
    {"732103", "*99#", "web.colombiamovil.com.co", "", ""},
    {"732103", "*99#", "web.uffmovil.com.co", "", ""},
    {"732103", "*99#", "une4glte.net.co", "", ""},
    {"732111", "*99#", "web.colombiamovil.com.co", "", ""},
    {"732123", "*99#", "internet.movistar.com.co", "movistar", "movistar"},
    {"732123", "*99#", "web.vmc.net.co", "", ""},
    {"732142", "*99#", "une4glte.net.co", "une", "une"},
    {"732165", "*99#", "web.colombiamovil.com.co", "", ""},
    {"7342", "*99#", "gprsweb.digitel.ve", "", ""},
    {"7344", "*99#", "internet.movistar.ve", "", ""},
    {"7344", "*99#", "pegaso.movistar.ve", "", ""},
    {"7346", "*99#", "int.movilnet.com.ve", "", ""},
    {"7346", "*99#", "intccs.movilnet.ve", "", ""},
    {"7346", "*99#", "intbto.movilnet.ve", "", ""},
    {"73602", "*99#", "4g.entel", "", ""},
    {"7400", "*99#", "navega.movistar.ec", "movistar", "movistar"},
    {"7401", "*99#", "internet.porta.com.ec", "", ""},
    {"7402", "*99#", "internet3g.alegro.net.ec", "", ""},
    {"7402", "*99#", "internet.cnt.net.ec", "", ""},
    {"7442", "*99#", "igprs.claro.com.py", "clarogprs", "clarogprs999"},
    {"7444", "*99#", "internet.tigo.py", "", ""},
    {"7445", "*99#", "internet", "personal", "personal"},
    {"7481", "*99#", "gprs.ancel", "", ""},
    {"7487", "*99#", "apnumt.movistar.com.uy", "movistar", "movistar"},
    {"74810", "*99#", "igprs.claro.com.uy", "clarogprs", "clarogprs999"},
    {"21601", "*99#", "digi", "", ""},
    {"21630", "*99#", "wnw", "", ""},
    {"21670", "*99#", "vitamax.snet.vodafone.net", "", ""},
    {"46001", "*99#", "3gnet", "any", "any"},
    {"46006", "*99#", "3gnet", "any", "any"},
    {"46003", "*99#", "ctnet", "ctnet@mycdma.cn", "vnet.mobi"},
    {"46005", "*99#", "ctnet", "ctnet@mycdma.cn", "vnet.mobi"},
    {"46011", "*99#", "ctnet", "ctnet@mycdma.cn", "vnet.mobi"},
    {"46000", "*99***1#", "cmnet", "any", "any"},
    {"46004", "*99***1#", "cmnet", "any", "any"},
    {"46007", "*99***1#", "cmnet", "any", "any"},
    {"46002", "*99***1#", "cmnet", "any", "any"},

    {"30222", "*99#", "isp.telus.com", "any", "any"},     // Canada Telus 302220
    {"30236", "*99#", "isp.telus.com", "any", "any"},     // Canada Telus
    {"30265", "*99#", "isp.telus.com", "any", "any"},     // Canada Telus
    {"30276", "*99#", "isp.telus.com", "any", "any"},     // Canada Telus
    {"30286", "*99#", "isp.telus.com", "any", "any"},     // Canada Telus
    {"30261", "*99#", "pda.bell.ca", "any", "any"},       // Canada Bell
    {"30264", "*99#", "pda.bell.ca", "any", "any"},       // Canada Bell
    {"30269", "*99#", "pda.bell.ca", "any", "any"},       // Canada Bell
    {"30232", "*99#", "internet.com", "wapuser1", "wap"}, // Canada Rogers
    {"30272", "*99#", "internet.com", "wapuser1", "wap"}, // Canada Rogers
    {"30282", "*99#", "internet.com", "wapuser1", "wap"}, // Canada Rogers
    {"30292", "*99#", "internet.com", "wapuser1", "wap"}, // Canada Rogers
    {"30250", "*99#", "media.videotron", "any", "any"},   // Canada Videotron
    {"30251", "*99#", "media.videotron", "any", "any"},   // Canada Videotron
    {"30252", "*99#", "media.videotron", "any", "any"},   // Canada Videotron

};

provider_table_t tables[] = {
    { providers_LTE, sizeof(providers_LTE)/sizeof(provider_t) },
    { providers_3G,  sizeof(providers_3G)/sizeof(provider_t)  }
};

AUTO_PROVIDER_LIST auto_provider_list;

void qcmap_release_apn(void) {
    memset(&auto_provider_list, 0, sizeof(auto_provider_list));
}

bool qcmap_set_apn_mccmnc_num(void) {
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    snprintf(command, QCMAP_MAX_STRING_LEN, "%s -i 2 %s=3,2", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_COPS);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        PRINT_DEBUG("result:[%s]\n", result);
        if (strcasestr(result, "OK")) {
            return true;
        }
    }

    return false;
}

bool qcmap_get_apn_mccmnc(void) {
    char *p = NULL;
    char *p1 = NULL;
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    if (!qcmap_set_apn_mccmnc_num())
        return false;

    snprintf(command, QCMAP_MAX_STRING_LEN, "%s  -i 2 %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_COPS);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        PRINT_DEBUG("result:[%s]\n", result);
        if (strcasestr(result, "OK")) {
            if ((p = strstr(result, ",\"")) != NULL) {
                p += 2;
                if ((p1 = strstr(p, "\"")) != NULL) {
                    *p1 = '\0';
                    // 安全拷贝并确保字符串终止
                    strncpy(glb_module_desc.apn_mccmnc, p, sizeof(glb_module_desc.apn_mccmnc) - 1);
                    glb_module_desc.apn_mccmnc[sizeof(glb_module_desc.apn_mccmnc) - 1] = '\0';
                    strncpy(glb_module_desc.rf.PLMN, p, sizeof(glb_module_desc.rf.PLMN) - 1);
                    glb_module_desc.rf.PLMN[sizeof(glb_module_desc.rf.PLMN) - 1] = '\0';
                    write_buf_to_file(PROVIDER_MCCMNC_FILE, glb_module_desc.apn_mccmnc, strlen(glb_module_desc.apn_mccmnc));
                    PRINT_DEBUG("get apn mccmnc success [%s]\n", glb_module_desc.apn_mccmnc);
                } else {
                    PRINT_DEBUG("MCCMNC closing quote not found\n");
                }
            } else {
                PRINT_DEBUG("MCCMNC pattern not found in response\n");
            }
            // 无论是否解析到MCCMNC，只要存在OK则返回true
            return true;
        }
    }

    return false;
}

bool qcmap_set_apn_mccmnc_str(void) {
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    snprintf(command, QCMAP_MAX_STRING_LEN, "%s -i 2 %s=3,0", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_COPS);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        if (strcasestr(result, "OK")) {
            return true;
        }
    }

    PRINT_DEBUG("set apn mccmnc str fail\n");
    return false;
}

bool qcmap_get_provider(void) {
    char *p = NULL;
    char *p1 = NULL;
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    if (!qcmap_set_apn_mccmnc_str())
        return false;

    snprintf(command, QCMAP_MAX_STRING_LEN, "%s -i 2 %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_COPS);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        if (strcasestr(result, "OK")) {
            if ((p = strstr(result, ",\"")) != NULL) {
                p += 2;
        
                if ((p1 = strstr(p, "\"")) != NULL) {
                    *p1 = '\0';
                    strncpy(glb_module_desc.apn_mccmnc, p, sizeof(glb_module_desc.apn_mccmnc) - 1);
                    write_buf_to_file(PROVIDER_RESULT_FILE, glb_module_desc.apn_mccmnc, strlen(glb_module_desc.apn_mccmnc));
                    PRINT_DEBUG("provider : [%s]\n", glb_module_desc.apn_mccmnc);
                    return true;
                }
            }
        }
    }
    PRINT_DEBUG("get provider fail\n");

    return false;
}

bool qcmap_get_sim_mccmnc(void) {

    if (glb_module_desc.imsi[0] == 0) {
        qcmap_get_imsi();
        if (glb_module_desc.imsi[0] == 0) return false;
    }

    // 优先匹配完整长度（≥6），其次短长度（<6）
    for (int is_short = 0; is_short <= 1; is_short++) 
    {
        for (size_t t = 0; t < sizeof(tables)/sizeof(tables[0]); t++) 
        {
            provider_t *plist = tables[t].providers;
            for (size_t i = 0; i < tables[t].count; i++) 
            {
                const char *current_mccmnc = plist[i].MCC_MNC;
                const size_t len = strlen(current_mccmnc);

                if (plist[i].apn[0] == '\0') continue;
                if ((is_short && len >= 6) || (!is_short && len < 6)) continue;

                char processed_mccmnc[8] = {0};
                if (len == 4) { // 处理短码补零逻辑
                    snprintf(processed_mccmnc, sizeof(processed_mccmnc), 
                            "%.3s0%c", current_mccmnc, current_mccmnc[3]);
                } else {
                    strncpy(processed_mccmnc, current_mccmnc, sizeof(processed_mccmnc)-1);
                }

                const size_t cmp_len = strlen(processed_mccmnc);
                if (strncmp(glb_module_desc.imsi, processed_mccmnc, cmp_len) == 0) 
                {
                    strncpy(glb_module_desc.sim_mccmnc, processed_mccmnc, sizeof(glb_module_desc.sim_mccmnc)-1);
                    strncpy(glb_module_desc.rf.PLMN, processed_mccmnc, sizeof(glb_module_desc.rf.PLMN) - 1);
                    write_buf_to_file(SIM_MCCMNC_FILE, glb_module_desc.sim_mccmnc, strlen(glb_module_desc.sim_mccmnc));
                    PRINT_DEBUG("Match: IMSI=%s, MCCMNC=%s (Processed: %s)\n", 
                              glb_module_desc.imsi, current_mccmnc, processed_mccmnc);
                    return true;
                }
            }
        }
    }

    // 无匹配时的默认处理
    if (glb_module_desc.imsi[0]) {
        strncpy(glb_module_desc.sim_mccmnc, glb_module_desc.imsi, 5);
        glb_module_desc.sim_mccmnc[5] = '\0';
        PRINT_DEBUG("No match, use IMSI prefix: %s\n", glb_module_desc.sim_mccmnc);
        return true;
    }

    return false;
}

bool qcmap_init_auto_apn_with_apn_mccmnc(void) {
    auto_provider_list.cont = 0;

    if (strlen(glb_module_desc.apn_mccmnc) <= 0)
        return false;

    for (size_t t = 0; t < sizeof(tables)/sizeof(tables[0]); t++) {
        provider_t *plist = tables[t].providers;
        for (size_t i = 0; i < tables[t].count; i++) {
            if (!strcmp(glb_module_desc.apn_mccmnc, plist[i].MCC_MNC)) {
                strcpy(auto_provider_list.auto_provider[auto_provider_list.cont].provider.MCC_MNC, plist[i].MCC_MNC);
                if (auto_provider_list.cont == 0) {
                    auto_provider_list.provider_use_index = 0;
                }

                auto_provider_list.auto_provider[auto_provider_list.cont].status = AUTO_APN_DEALUTL;

                if (strcmp(plist[i].apn, "") == 0) {
                    strcpy(auto_provider_list.auto_provider[auto_provider_list.cont].provider.apn, "internet");
                } else {
                    strcpy(auto_provider_list.auto_provider[auto_provider_list.cont].provider.apn, plist[i].apn);
                }

                if (strcmp(plist[i].username, "") == 0) {
                    strcpy(auto_provider_list.auto_provider[auto_provider_list.cont].provider.username, "any");
                } else {
                    strcpy(auto_provider_list.auto_provider[auto_provider_list.cont].provider.username, plist[i].username);
                }

                if (strcmp(plist[i].password, "") == 0) {
                    strcpy(auto_provider_list.auto_provider[auto_provider_list.cont].provider.password, "any");
                } else {
                    strcpy(auto_provider_list.auto_provider[auto_provider_list.cont].provider.password, plist[i].password);
                }
                PRINT_DEBUG("tables [%d] auto apn match, apn [%s] username [%s] password [%s]\n", t, 
                    auto_provider_list.auto_provider[auto_provider_list.cont].provider.apn, 
                    auto_provider_list.auto_provider[auto_provider_list.cont].provider.username, 
                    auto_provider_list.auto_provider[auto_provider_list.cont].provider.password);
                if (auto_provider_list.cont++ == 64)
                    return true;
            }
        }
    }

    return true;
}

bool qcmap_init_auto_apn_with_sim_mccmnc(void) {
    auto_provider_list.cont = 0;

    if (strlen(glb_module_desc.sim_mccmnc) <= 0)
        return false;

    for (size_t t = 0; t < sizeof(tables)/sizeof(tables[0]); t++) {
        provider_t *plist = tables[t].providers;
        for (size_t i = 0; i < tables[t].count; i++) {
            if (!strcmp(glb_module_desc.sim_mccmnc, plist[i].MCC_MNC)) {
                strcpy(auto_provider_list.auto_provider[auto_provider_list.cont].provider.MCC_MNC, plist[i].MCC_MNC);
                if (auto_provider_list.cont == 0) {
                    auto_provider_list.provider_use_index = 0;
                }

                auto_provider_list.auto_provider[auto_provider_list.cont].status = AUTO_APN_DEALUTL;

                if (strcmp(plist[i].apn, "") == 0) {
                    strcpy(auto_provider_list.auto_provider[auto_provider_list.cont].provider.apn, "internet");
                } else {
                    strcpy(auto_provider_list.auto_provider[auto_provider_list.cont].provider.apn, plist[i].apn);
                }

                if (strcmp(plist[i].username, "") == 0) {
                    strcpy(auto_provider_list.auto_provider[auto_provider_list.cont].provider.username, "any");
                } else {
                    strcpy(auto_provider_list.auto_provider[auto_provider_list.cont].provider.username, plist[i].username);
                }

                if (strcmp(plist[i].password, "") == 0) {
                    strcpy(auto_provider_list.auto_provider[auto_provider_list.cont].provider.password, "any");
                } else {
                    strcpy(auto_provider_list.auto_provider[auto_provider_list.cont].provider.password, plist[i].password);
                }
                PRINT_DEBUG("tables [%d] auto apn match, apn [%s] username [%s] password [%s]\n", t, 
                    auto_provider_list.auto_provider[auto_provider_list.cont].provider.apn, 
                    auto_provider_list.auto_provider[auto_provider_list.cont].provider.username, 
                    auto_provider_list.auto_provider[auto_provider_list.cont].provider.password);
                if (auto_provider_list.cont++ == 64)
                    return true;
            }
        }
    }

    return true;
}

bool qcmap_init_auto_apn_with_no_mccmnc(void) {
    char apn[32] = {0};

    PRINT_DEBUG("no mccmnc(plmn), auto apn get from modem\n");
    qcmap_get_apn_by_index(1, apn, sizeof(apn));
    if (strlen(apn) <= 0) {
        PRINT_DEBUG("no auto apn in modem\n");
        return false;
    }

    auto_provider_list.cont = 1;
    auto_provider_list.provider_use_index = 0;
    auto_provider_list.auto_provider[0].status = AUTO_APN_DEALUTL;
    strcpy(auto_provider_list.auto_provider[0].provider.apn, apn);
    strcpy(auto_provider_list.auto_provider[0].provider.username, "any");
    strcpy(auto_provider_list.auto_provider[0].provider.password, "any");
    PRINT_DEBUG("get apn [%s] from modem\n", auto_provider_list.auto_provider[0].provider.apn);
    return true;
}

void qcmap_init_auto_apn_list(void) {
    int ret = 0;
    memset(&auto_provider_list, 0, sizeof(auto_provider_list));

    RETRY_UNTIL_SUCCESS(qcmap_get_apn_mccmnc, 5, 1);
    RETRY_UNTIL_SUCCESS(qcmap_get_sim_mccmnc, 5, 1);

    ret = RETRY_UNTIL_SUCCESS(qcmap_init_auto_apn_with_apn_mccmnc, 5, 1);
    if (!ret) {
        ret = RETRY_UNTIL_SUCCESS(qcmap_init_auto_apn_with_sim_mccmnc, 5, 1);
    }

    if (ret) {
        PRINT_DEBUG("auto apn list :\n");
        for (int i = 0; i < auto_provider_list.cont; i++) {
            PRINT_DEBUG("[%d] mccmnc [%s] apn [%s] username [%s] password [%s]\n", i, 
                auto_provider_list.auto_provider[i].provider.MCC_MNC, 
                auto_provider_list.auto_provider[i].provider.apn, 
                auto_provider_list.auto_provider[i].provider.username,
                auto_provider_list.auto_provider[i].provider.password);
        }
    } else {
        PRINT_DEBUG("init auto apn list fail\n");
        ret = qcmap_init_auto_apn_with_no_mccmnc();
        if (!ret && qcmap_wan5g_list[0].enable && !qcmap_wan5g_list[0].manaul_apn) {
            qcmap_enable_state_machine();
            qcmap_reset_signal();
        }
    }
}

void qcmap_update_auto_apn_status(int status) {
    char autoapninfos[512] = {0};

    switch (status)
    {
    case AUTO_APN_VALID:
        auto_provider_list.auto_provider[auto_provider_list.provider_use_index].status = AUTO_APN_VALID;
        PRINT_DEBUG("mccmnc [%s] apn [%s] username [%s] password [%s] is valid\n", 
            auto_provider_list.auto_provider[auto_provider_list.provider_use_index].provider.MCC_MNC,
            auto_provider_list.auto_provider[auto_provider_list.provider_use_index].provider.apn, 
            auto_provider_list.auto_provider[auto_provider_list.provider_use_index].provider.username,
            auto_provider_list.auto_provider[auto_provider_list.provider_use_index].provider.password);
        sprintf(autoapninfos, "%s %s %s", 
            auto_provider_list.auto_provider[auto_provider_list.provider_use_index].provider.apn, 
            auto_provider_list.auto_provider[auto_provider_list.provider_use_index].provider.username,
            auto_provider_list.auto_provider[auto_provider_list.provider_use_index].provider.password);
        write_buf_to_file(AUTOAPN_INFOS_FILE, autoapninfos, sizeof(autoapninfos));
        break;
    case AUTO_APN_INVALID:
        auto_provider_list.auto_provider[auto_provider_list.provider_use_index].status = AUTO_APN_INVALID;
        PRINT_DEBUG("mccmnc [%s] apn [%s] username [%s] password [%s] is invalid\n", 
            auto_provider_list.auto_provider[auto_provider_list.provider_use_index].provider.MCC_MNC,
            auto_provider_list.auto_provider[auto_provider_list.provider_use_index].provider.apn, 
            auto_provider_list.auto_provider[auto_provider_list.provider_use_index].provider.username,
            auto_provider_list.auto_provider[auto_provider_list.provider_use_index].provider.password);
        if (auto_provider_list.provider_use_index == auto_provider_list.cont - 1) {
            PRINT_DEBUG("there are no useful auto apn\n");
            break;
        }
        auto_provider_list.provider_use_index++;
        PRINT_DEBUG("move to next auto apn: mccmnc [%s] apn [%s] username [%s] password [%s]\n", 
            auto_provider_list.auto_provider[auto_provider_list.provider_use_index].provider.MCC_MNC,
            auto_provider_list.auto_provider[auto_provider_list.provider_use_index].provider.apn, 
            auto_provider_list.auto_provider[auto_provider_list.provider_use_index].provider.username,
            auto_provider_list.auto_provider[auto_provider_list.provider_use_index].provider.password);
        break;
    default:
        break;
    }
}

provider_t *qcmap_get_auto_apn(void) {
    return &(auto_provider_list.auto_provider[auto_provider_list.provider_use_index].provider);
}

bool qcmap_update_apn_name(int profile_index, int config_index) {
    char apn_name[32] = {0};
    provider_t *auto_apn = nullptr;
    qcmap_net_policy_info net_policy;
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;

    //切换到该profile
    if (!qcmap_client->SetWWANProfileHandlePreference(profile_index, &qmi_err_num)) {
        PRINT_DEBUG("change to profile [%d] config [%d] fail\n", profile_index, config_index);
        return false;
    }

    memset(&net_policy,0,sizeof(qcmap_msgr_net_policy_info_v01));
    if (profile_index == 1 && !qcmap_wan5g_list[0].manaul_apn) {
        auto_apn = qcmap_get_auto_apn();
        if (auto_apn) {
            sprintf(net_policy.apn_name, "%s", auto_apn->apn);
        } else {
            PRINT_DEBUG("get profile [%d] config [%d] auto apnname fail\n", profile_index, config_index);
            return false;
        }
    } else {
        sprintf(net_policy.apn_name, "%s", qcmap_wan5g_list[config_index].apn_name);
    }

    if (qcmap_get_apn_by_index(profile_index, apn_name, sizeof(apn_name))) {
        if (!strcasecmp(apn_name, "ims")) {
            PRINT_DEBUG("profile [%d] config [%d] can not change apnname [%s] to [%s]\n", profile_index, config_index, apn_name);
            return true;
        }
    
        if (!strcasecmp(apn_name, "sos")) {
            PRINT_DEBUG("profile [%d] config [%d] can not change apnname [%s] to [%s]\n", profile_index, config_index, apn_name);
            return true;
        }
    }

    if (qcmap_client->UpdateWWANPolicyEx(QCMAP_MSGR_UPDATE_APN_NAME_V01, net_policy, &qmi_err_num)) {
        PRINT_DEBUG("update profile [%d] config [%d] apnname [%s] success\n", profile_index, config_index, net_policy.apn_name);
        qcmap_show_profile(profile_index);
    } else {
        PRINT_DEBUG("update profile [%d] config [%d] apnname [%s] fail\n", profile_index, config_index, net_policy.apn_name);
        return false;
    }

    return true;
}

bool qcmap_sync_apn_info_with_modem(int profile_index, int config_index) {
    char apn_name[32] = {0};
    provider_t *auto_apn = nullptr;
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    if ((profile_index == 1 && work_type == WORK_TYPE_MULTI) || (profile_index > 1 && work_type == WORK_TYPE_BASIC))
        return;

    if (profile_index == 1 && !qcmap_wan5g_list[0].manaul_apn) {
        auto_apn = qcmap_get_auto_apn();
        if (auto_apn) {
            snprintf(command, QCMAP_MAX_STRING_LEN, "%s -i 1 \'%s=%d,\"%s\",\"%s\"\'", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CGDCONT, profile_index, IP_FAMILY_TYPE_STR(qcmap_wan5g_list[config_index].ip_family), auto_apn->apn);
        } else {
            PRINT_DEBUG("get profile [%d] config [%d] auto apn info fail\n", profile_index, config_index);
            return false;
        }
    } else {
        snprintf(command, QCMAP_MAX_STRING_LEN, "%s \'%s=%d,\"%s\",\"%s\"\'", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CGDCONT, profile_index, IP_FAMILY_TYPE_STR(qcmap_wan5g_list[config_index].ip_family), qcmap_wan5g_list[config_index].apn_name);
    }

    if (qcmap_get_apn_by_index(profile_index, apn_name, sizeof(apn_name))) {
        if (!strcasecmp(apn_name, "ims")) {
            PRINT_DEBUG("profile [%d] config [%d] can not change apnname [%s]\n", profile_index, config_index, apn_name);
            return true;
        }
    
        if (!strcasecmp(apn_name, "sos")) {
            PRINT_DEBUG("profile [%d] config [%d] can not change apnname [%s]\n", profile_index, config_index, apn_name);
            return true;
        }
    }

    if (execute_cmd((const char*)command, result, sizeof(result))) {
        PRINT_DEBUG("result:[%s]\n", result);
        if (strcasestr(result, "OK")) {
            PRINT_DEBUG("set modem apn[%d] config [%d] info success, move to next step\n", profile_index, config_index);
            return true;
        }
    }

    PRINT_DEBUG("set modem apn[%d] config [%d] info fail, move to next step\n", profile_index, config_index);
    return false;
}

bool qcmap_sync_apn_auth_with_modem(int profile_index, int config_index) {
    provider_t *auto_apn = nullptr;
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    if ((profile_index == 1 && work_type == WORK_TYPE_MULTI) || (profile_index > 1 && work_type == WORK_TYPE_BASIC))
        return;

    if (profile_index == 1 && !qcmap_wan5g_list[0].manaul_apn) {
        auto_apn = qcmap_get_auto_apn();
        if (auto_apn) {
            snprintf(command, QCMAP_MAX_STRING_LEN, "%s -i 1 \'%s=%d,%d,\"%s\",\"%s\"'", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CGAUTH, profile_index, 
                qcmap_wan5g_list[config_index].auth, 
                auto_apn->username, 
                auto_apn->password);
        } else {
            PRINT_DEBUG("get profile [%d] config [%d] auto apnname fail\n", profile_index, config_index);
            return false;
        }
    } else {
        snprintf(command, QCMAP_MAX_STRING_LEN, "%s \'%s=%d,%d,\"%s\",\"%s\"'", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CGAUTH, profile_index, 
            qcmap_wan5g_list[config_index].auth, 
            qcmap_wan5g_list[config_index].username, 
            qcmap_wan5g_list[config_index].password);
    }

    if (execute_cmd((const char*)command, result, sizeof(result))) {
        PRINT_DEBUG("result:[%s]\n", result);
        if (strcasestr(result, "OK")) {
            PRINT_DEBUG("set modem apn[%d] config [%d] auth success, move to next step\n", profile_index, config_index);
            return true;
        }
    }

    PRINT_DEBUG("set modem apn[%d] config [%d] auth fail, move to next step\n", profile_index, config_index);
    return false;
}

bool qcmap_get_apn_auth_info(int config_index, qcmap_wan5g_config_t *config) {
    FILE* fd = NULL;
    char line[128] = {0};
    char* ptr = NULL;
    char cmd[128] = {0};
    char dest_str[32] = {0};
    bool ret = false;

    sprintf(dest_str, "+CGAUTH: %d", config_index + 1);
    sprintf(cmd, "%s %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CGAUTH);
    if ((fd = popen(cmd, "r")) != NULL) {
        while (fgets(line, sizeof(line) - 1, fd)) {
            if (strstr(line, dest_str) != NULL) {
                sscanf(line, "+CGAUTH: %d,%d,\"%31[^\"]\",\"%31[^\"]\"",
                    &config->profile_index, &config->auth, config->username, config->password);
                PRINT_DEBUG("profile id [%d], auth [%d], username [%s], password [%s]\n", 
                    config->profile_index, config->auth, config->username, config->password);
                ret = true;
                break;
            }
        }
        pclose(fd);
    }

    return ret;
}

/*
//获取可设置鉴权的apn数量
bool qcmap_init_auth_apn_num(void) {
    FILE* fd = NULL;
    int apn_num = 0;
    bool ret = false;
    char result[QCMAP_MAX_STRING_LEN * 2] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    sprintf(command, "%s -i 30 %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CGAUTH);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        if (strcasestr(result, "OK")) {
            ret = true;
            const char *pos = result;
            const char *target = "+CGAUTH:";
            // 遍历字符串查找目标子串 
            while ((pos = strstr(pos, target)) != NULL) {
                apn_num++;
                pos += strlen(target); // 移动指针到匹配位置之后 
            }
            PRINT_DEBUG("result:[%s]\n", result);
        }
    }

    if (ret) {
        g_apn_auth_num = apn_num;
        PRINT_DEBUG("init support setting auth apn num [%d]\n", g_apn_auth_num);
    }

    return ret;
}
*/

int qcmap_get_apn_num(void) {
    FILE* fd = NULL;
    char line[128] = {0};
    char cmd[128] = {0};
    int profile_index = 0;
    char apn_name[32] = {0};

    sprintf(cmd, "%s %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CGDCONT);
    if ((fd = popen(cmd, "r")) == NULL) {
        PRINT_DEBUG("Failed to execute AT command [%s]\n", cmd);
        return profile_index;
    }

    while (fgets(line, sizeof(line), fd) != NULL) {
        if (strstr(line, "+CGDCONT:") == line) {
            profile_index++;
        }
    }
    pclose(fd);

    PRINT_DEBUG("get apn num [%d]\n", profile_index);
    return profile_index;
}

int qcmap_get_max_apn_num_with_sim(void) {
    FILE* fd = NULL;
    char line[128] = {0};
    char cmd[128] = {0};
    int profile_index = 0;
    int profile_index_ex = 0;
    char apn_name[32] = {0};

    sprintf(cmd, "%s -i 10 %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CGDCONT);
    if ((fd = popen(cmd, "r")) == NULL) {
        PRINT_DEBUG("Failed to execute AT command [%s]\n", cmd);
        return profile_index;
    }

    while (fgets(line, sizeof(line), fd) != NULL) {
        if (strstr(line, "+CGDCONT:") == line) {
            memset(apn_name, 0, sizeof(apn_name));
            sscanf(line, "+CGDCONT: %d,\"%*[^\"]\",\"%31[^\"]\"", &profile_index, apn_name);
            if (strlen(apn_name) > 0 && !strcasecmp(apn_name, "sos")) {
                profile_index_ex = profile_index;
                PRINT_DEBUG("profile_index_sos:[%d], apn:[%s]\n", profile_index_ex, apn_name);
            }
            if (strlen(apn_name) > 0 && !strcasecmp(apn_name, "ims")) {
                profile_index_ex = profile_index;
                PRINT_DEBUG("profile_index_ims:[%d], apn:[%s]\n", profile_index_ex, apn_name);
            }
        }
    }
    pclose(fd);

    PRINT_DEBUG("get max apn num with sim[%d]\n", profile_index_ex);
    return profile_index_ex;
}

bool qcmap_get_apn_by_index(int profile_index, char *apn_name, int name_lan) {
    bool ret = false;
    FILE* fd = NULL;
    char line[128] = {0};
    char cmd[128] = {0};
    int index = 0;

    sprintf(cmd, "%s %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CGDCONT);
    if ((fd = popen(cmd, "r")) == NULL) {
        PRINT_DEBUG("Failed to execute AT command [%s]\n", cmd);
        return ret;
    }

    while (fgets(line, sizeof(line), fd) != NULL) {
        if (strstr(line, "+CGDCONT:") == line) {
            memset(apn_name, 0, name_lan);
            sscanf(line, "+CGDCONT: %d,\"%*[^\"]\",\"%31[^\"]\"", &index, apn_name);
            if (strlen(apn_name) > 0 && profile_index == index) {
                PRINT_DEBUG("index:[%d], apn:[%s]\n", index, apn_name);
                ret = true;
                break;
            }
        }
    }
    pclose(fd);

    return ret;
}

bool qcmap_init_apn_num(void) {
    bool add_flag = false;
    g_apn_num = 1;
    g_max_apn_num = 1;
    //g_apn_auth_num = 0;
    g_apn_num = qcmap_get_max_apn_num_with_sim();
    g_max_apn_num =  g_apn_num + QCMAP_MNGR_MULTI_APN_NUM;
    PRINT_DEBUG("init original apn num [%d] max apn num: [%d]\n", g_apn_num, g_max_apn_num);

    add_flag = qcmap_init_apn_num_in_modem();
    // if (!add_flag)
    //     RETRY_UNTIL_SUCCESS(qcmap_init_auth_apn_num, RETRY_TIMES, SLEEP_TIME);

    return add_flag;
}

bool qcmap_init_apn_num_in_modem(void) {
    bool add_flag = false;
    int count = 0;
    int apn_num = 0;
    char command[QCMAP_MAX_STRING_LEN] = {0};
    char result[QCMAP_MAX_STRING_LEN] = {0};

    apn_num = qcmap_get_apn_num();
    if (apn_num >= g_max_apn_num)
        return add_flag;

    for(int i = apn_num; i < g_max_apn_num; ) {
        memset(command, 0, sizeof(command));
        snprintf(command, QCMAP_MAX_STRING_LEN, "%s \'%s=%d,\"%s\",\"%s\"\'", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CGDCONT, i + 1, "IPV4V6", "temp");
        if (execute_cmd((const char*)command, result, sizeof(result))) {
            PRINT_DEBUG("result:[%s]\n", result);
            if (strcasestr(result, "OK")) {
                PRINT_DEBUG("add apn[%d] success\n", i + 1);
                i++;
                add_flag = true;
            } else {
                PRINT_DEBUG("add apn[%d] fail, try again\n", i + 1);
            }
        }
        count++;
        if (i < (g_max_apn_num - apn_num) && count == 10) {
            PRINT_DEBUG("init apn in modem with sim fail, exit\n");
            qcmap_signal_handler(SIGTERM);
            PRINT_DEBUG("------------------------>>> qcmap_mngr end ------------------------>>>\n");
            exit(1);
        }
        usleep(0.5 * 1000 * 1000);
    }

    if (add_flag) {
        qcmap_init_cfun(true);
    }

    return add_flag;
}