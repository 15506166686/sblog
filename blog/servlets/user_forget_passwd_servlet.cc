#include "user_forget_passwd_servlet.h"
#include "blog/util.h"
#include "blog/struct.h"
#include "blog/manager/user_manager.h"
#include "sylar/email/smtp.h"

namespace blog {
namespace servlet {

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

UserForgetPasswdServlet::UserForgetPasswdServlet()
    :BlogServlet("UserForgetPasswd") {
}

int32_t UserForgetPasswdServlet::handle(sylar::http::HttpRequest::ptr request
                                  ,sylar::http::HttpResponse::ptr response
                                  ,sylar::http::HttpSession::ptr session
                                  ,Result::ptr result) {
    do {
        DEFINE_AND_CHECK_STRING(result, email, "email");

        data::UserInfo::ptr info;
        if(is_email(email)) {
            info = UserMgr::GetInstance()->getByEmail(email);
        } else {
            result->setResult(402, "invalid email");
            break;
        }
        if(!info) {
            result->setResult(403, "email not register");
            break;
        }
        auto v = sylar::random_string(16);
        info->setCode(v);
        auto db = getDB();
        //auto trans = db->openTransaction();
        if(data::UserInfoDao::Update(info, db)) {
            result->setResult(500, "db update error");
            break;
        }
        std::string send_email = sylar::EnvMgr::GetInstance()->getEnv("blog_email");
        std::string send_email_passwd = sylar::EnvMgr::GetInstance()->getEnv("blog_email_passwd");
        auto mail = sylar::EMail::Create(
                send_email, send_email_passwd
                ,"SBlog 重置密码 - 验证码"
                ,"验证码[" + v + "]"
                ,{email}, {}, {send_email});
        auto client = sylar::SmtpClient::Create("smtp.163.com", 25);
        if(!client) {
            SYLAR_LOG_ERROR(g_logger) << "connect email server fail";
            result->setResult(501, "connect email server fail");
            break;
        }

        auto r = client->send(mail, 5000);
        if(r->result != 0) {
            result->setResult(501, std::to_string(r->result) + " " + r->msg);
            break;
        }
        //trans->commit();
        result->setResult(200, "ok");
    } while(false);
    response->setBody(result->toJsonString());
    return 0;
}

}
}