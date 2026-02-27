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
	for (size_t i = 0; i < env.size(); ++i)
		envp.push_back(const_cast<char *>(env[i].c_str()));
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
    std::vector<std::string> exts = config.getCgiConf().extensions;
	for (size_t i = 0; i < exts.size(); i++)
	{
		if (exts[i] == ext)
			return true;
	}
	return false;
}

bool cgiHandler::fileExist(std::string const &path) const
{
	if (access(path.c_str(), F_OK | X_OK) == 0)
		return true;
	return false;
}

void cgiHandler::run(std::string const &path)
{
    int pipe_to_child[2];
    int pipe_from_child[2];
    if (pipe(pipe_to_child) < 0)
        return;
    if (pipe(pipe_from_child) < 0)
    {
        close(pipe_to_child[0]);
        close(pipe_to_child[1]);
        return;
    }
    pid_t pid = fork();
    if (pid < 0)
    {
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
        execScrypt(path);
        exit(1);
    }
    else
    {
        close(pipe_to_child[0]);
        close(pipe_from_child[1]);
        std::string input_data = request.body.content;
        if (!input_data.empty())
            write(pipe_to_child[1], input_data.c_str(), input_data.length());
        close(pipe_to_child[1]);
        char buffer[4096];
        ssize_t bytes_read;
        while ((bytes_read = read(pipe_from_child[0], buffer, sizeof(buffer) - 1)) > 0)
            sendBuff.append(buffer, bytes_read);
        close(pipe_from_child[0]);
        int status;
        waitpid(pid, &status, 0);
        if (status == 0 && !sendBuff.empty())
            success = true;
    }
}

bool cgiHandler::isSuccess(void) const {return success;}

std::string const &cgiHandler::getSendBuff(void) const {return sendBuff;}

void cgiHandler::execScrypt(std::string const &path)
{
    std::string ext = path.substr(path.rfind(".") + 1);
    char *argv[2];
    argv[0] = const_cast<char*>(path.c_str());
    argv[1] = NULL;
    prepareEnvp();
    execve(path.c_str(), argv, envp.data());
}
