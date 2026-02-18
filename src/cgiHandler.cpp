#include "../includes/cgiHandler.hpp"

cgiHandler::cgiHandler(serverConfig const &conf, t_request const &req) : config(conf), request(req), success(false) {}

cgiHandler::~cgiHandler(void) {}

void cgiHandler::setEnv(std::string const &page)
{
	std::map<std::string, std::string>::iterator it = request.headers.begin();
	std::map<std::string, std::string>envVars = config.getEnv();
	for (; it != request.headers.end(); ++it)
	{
		std::map<std::string, std::string>::iterator found = envVars.find(it->first);
		if (found != envVars.end())
			env.push_back(found->second + "=" + it->second);
	}
	env.push_back("QUERY_STRING=" + request.query);
	env.push_back("REQUEST_METHOD=" + request.method);
	env.push_back("SCRIPT_NAME=" + page);
}

void cgiHandler::prepareEnvp(void)
{
	envp.clear();
	for (size_t i = 0; i < env.size(); ++i)
		envp.push_back(const_cast<char*>(env[i].c_str()));
	envp.push_back(NULL);
}

bool cgiHandler::isCgiAllowed() const
{
	t_cgi cgiConf = config.getCgiConf();
	if (cgiConf.cgiAllowed != 1)
		return false;
	return true;
}

std::string cgiHandler::getPage(t_route const &route) const
{
	std::string page = request.page;
	if (page.empty())
		page = route.page;
	return page;
}

bool cgiHandler::checkPageExtension(std::string const &page) const
{
	if (page.empty())
		return false;
	std::string ext = page.substr(page.rfind(".") + 1);
	std::vector<std::string>::const_iterator it = config.getCgiConf().extensions.begin();
	for (; it != config.getCgiConf().extensions.end(); ++it)
	{
		if (*it == ext)
			return true;
	}
	return false;
}

bool cgiHandler::fileExist(std::string const &path) const
{
	if (access(path.c_str(), F_OK) == 0)
		return true;
	return false;
}

void cgiHandler::run(std::string const &path)
{
    int pipe_to_child[2];
    int pipe_from_child[2];
    if (pipe(pipe_to_child) < 0) {
        throw errorHandler("Error creating pipe_to_child");
        return;
    }
    if (pipe(pipe_from_child) < 0) {
        throw errorHandler("Error creating pipe_from_child");
        close(pipe_to_child[0]);
        close(pipe_to_child[1]);
        return;
    }
    pid_t pid = fork();
    if (pid < 0) {
        throw errorHandler("Error forking process");
        close(pipe_to_child[0]);
        close(pipe_to_child[1]);
        close(pipe_from_child[0]);
        close(pipe_from_child[1]);
        return;
    }
    if (pid == 0) 
    {
        close(pipe_to_child[1]); 
        dup2(pipe_to_child[0], STDIN_FILENO);
        close(pipe_to_child[0]);
        close(pipe_from_child[0]);
        dup2(pipe_from_child[1], STDOUT_FILENO);
        close(pipe_from_child[1]);
        std::string ext = path.substr(path.rfind(".") + 1);
        if (ext == "py")
            execl("/usr/bin/python3", "python3", path.c_str(), NULL);
        else if (ext == "php")
            execl("/usr/bin/php", "php", path.c_str(), NULL);
        else if (ext == "cgi")
            execl(path.c_str(), path.c_str(), NULL);
        exit(1);
    } 
    else 
    {
        close(pipe_to_child[0]);
        close(pipe_from_child[1]);
        std::string input_data = request.body.content;
        write(pipe_to_child[1], input_data.c_str(), input_data.length());
        close(pipe_to_child[1]);
        char buffer[4096];
        ssize_t bytes_read;
        std::string output;
        while ((bytes_read = read(pipe_from_child[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[bytes_read] = '\0';
            output += buffer;
        }
        close(pipe_from_child[0]);
        int status;
        waitpid(pid, &status, 0);
        if (status == 0 && !output.empty())
        {
            success = true;
            sendBuff = output;
        }
    }
}

bool cgiHandler::isSuccess(void) const {return success;}

std::string const &cgiHandler::getSendBuff(void) const {return sendBuff;}
