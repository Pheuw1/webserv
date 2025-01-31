#include "webserv.hpp"
#include "server.hpp"

typedef vector<Server>::iterator IT;
typedef vector<string>::iterator Names;
typedef vector<Response>::iterator Rep;

WebServ::~WebServ() {;}

WebServ::WebServ(string config_path, char **env) {
      init(env);
      vector<string> settings =  parse_config(config_path);
      vector<vector<string> >routes;
      vector<vector<string> >dirs;
      vector <string> scope;
      for (vector<string>::iterator it = settings.begin(); it != settings.end(); it++) {
           
            scope = get_all_scopes(*it, "routes", "[", "]");
            if (scope.size() > 1)
                  throw invalid_argument("only one route block allowed per server");
            if (scope.size())
                  routes = parse_routes(scope[0], *this);
            for (size_t i = 0; i < routes.size(); i++) {
                  routes[i][0] = root + "/" + routes[i][0];
                  if (!dir_exist(routes[i][0].data()) && access(routes[i][0].data(), R_OK) < 0)
                        throw invalid_argument("invalid route: " + routes[i][0]);
                  for (size_t j = 1; j < routes[i].size(); j++)
                        if (Methods.find(routes[i][j]) == Methods.end())
                              throw invalid_argument("invalid method "+ routes[i][j] + "for route: " + routes[i][0]);
            }
            if (routes.size())
                  it->erase(it->find("routes"), it->find("]"));
            scope = get_all_scopes(*it, "dirs", "[", "]");
            if (scope.size() > 1)
                  throw invalid_argument("only one directory block allowed per server");
            if (scope.size())
                  dirs = parse_routes(scope[0], *this);
            for (size_t i = 0; i < dirs.size(); i++) {
                  cout << dirs[i].size() <<  ":"<< dirs[i][0] << endl;
                  if (dirs[i].size() != 2)
                        throw invalid_argument("declaration of dir component requires one argument");
                  if (!dir_exist((root + "/"+ dirs[i][0]).data()))
                        throw invalid_argument("invalid dir: " + dirs[i][0]);
                  if (access((root + "/"+  dirs[i][1]).data(), R_OK) < 0)
                        throw invalid_argument("invalid path " + dirs[i][1] + " for dir: " + dirs[i][0]);
            }
            if (dirs.size())
                  it->erase(it->find("dir"), it->find("]"));

            servers.push_back(Server(parse_scope(*it, *this), routes, dirs, this));
            routes.clear();
            dirs.clear();
      }
}

Server *WebServ::get_host(string name, int port) {
      for (IT it = servers.begin(); it  !=servers.end(); it++)
            for (Names names = it->names.begin(); names != it->names.end(); names++)
                  if (name == *names)
                        return (&(*it));
      for (IT it = servers.begin(); it  !=servers.end(); it++)
            for (Server::IT s = it->sockets.begin(); s !=  it->sockets.end();s++)
                  if (s->port == port)
                        return (&(*it));
      return (NULL);
}

void WebServ::run() {
      while (true) {
            requests.clear();
            max_sd = 0; FD_ZERO(&readfds); FD_ZERO(&writefds);   
            for (IT it = servers.begin(); it  !=servers.end(); it++)
                  max_sd = max(it->check_ready(readfds, writefds), max_sd);
            if (select(max_sd + 1, &readfds ,&writefds , NULL , NULL) <0)
                  throw Socket::connect_except();
            for (std::vector<Server>::iterator it = servers.begin(); it  !=servers.end(); it++)//responses are set now
                  it->get_requests(readfds, writefds, this);
            for (Rep it = responses.begin(); it != responses.end(); it++){
                  if (FD_ISSET(it->req_cp.sd , &writefds)){
                        if (send(it->req_cp.sd, it->buffer.str().data(), it->buffer.str().size(), 0) <= 0)
                              throw Socket::connect_except();
                        responses.erase(it--);
                  }
            }
      }
}

/*this is sad and shameful*/
void WebServ::init(char **e) {
      char tmp[2048];
      cwd = string(getcwd(tmp, 2048));
      cout << cwd << endl;
      env = (char * const *)e;     
      root = "www"; 
      home = "HTML/home.html";
      error_path = "errors/";
      Methods.insert(make_pair("GET",    &GET));
      Methods.insert(make_pair("POST",   &POST));
      Methods.insert(make_pair("DELETE", &DELETE));
      cgi_exts.insert("php");
      cgi_exts.insert("py");
      HttpStatusCode.insert(make_pair(100 ,"Continue"));
      HttpStatusCode.insert(make_pair(101 ,"SwitchingProtocols"));
      HttpStatusCode.insert(make_pair(103 ,"EarlyHints"));
      HttpStatusCode.insert(make_pair(200 ,"Ok"));
      HttpStatusCode.insert(make_pair(201 ,"Created"));
      HttpStatusCode.insert(make_pair(202 ,"Accepted"));
      HttpStatusCode.insert(make_pair(203 ,"NonAuthoritativeInformation"));
      HttpStatusCode.insert(make_pair(204 ,"NoContent"));
      HttpStatusCode.insert(make_pair(205 ,"ResetContent"));
      HttpStatusCode.insert(make_pair(206 ,"PartialContent"));
      HttpStatusCode.insert(make_pair(300 ,"MultipleChoices"));
      HttpStatusCode.insert(make_pair(301 ,"MovedPermanently"));
      HttpStatusCode.insert(make_pair(302 ,"Found"));
      HttpStatusCode.insert(make_pair(304 ,"NotModified"));
      HttpStatusCode.insert(make_pair(400 ,"BadRequest"));
      HttpStatusCode.insert(make_pair(401 ,"Unauthorized"));
      HttpStatusCode.insert(make_pair(403 ,"Forbidden"));
      HttpStatusCode.insert(make_pair(404 ,"NotFound"));
      HttpStatusCode.insert(make_pair(405 ,"MethodNotAllowed"));
      HttpStatusCode.insert(make_pair(408 ,"RequestTimeout"));
      HttpStatusCode.insert(make_pair(418 ,"ImATeapot"));
      HttpStatusCode.insert(make_pair(500 ,"InternalServerError"));
      HttpStatusCode.insert(make_pair(501 ,"NotImplemented"));
      HttpStatusCode.insert(make_pair(502 ,"BadGateway"));
      HttpStatusCode.insert(make_pair(503 ,"ServiceUnvailable"));
      HttpStatusCode.insert(make_pair(504 ,"GatewayTimeout"));
      HttpStatusCode.insert(make_pair(505 ,"HttpVersionNotSupported"));
}