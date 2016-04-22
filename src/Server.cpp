#define _HAVE_SSL_SUPPORT 1
#include <ape_netlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <list>
#ifdef __linux__
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#endif

#include "Server.h"
#include "Context.h"
#include "Macros.h"
#include "REPL.h"

#include <Binding/JSProcess.h>

unsigned long _ape_seed;

namespace Nidium {
namespace Server {

#define NIDIUM_SERVER_VERSION "0.2-dev"
#define NIDIUM_MAX_WORKERS 64

static std::list<pid_t> pidList;

static void signal_handler(int sign)
{
    APE_loop_stop();
    for (auto it : pidList) {
        kill(it, sign);
    }
}

static int inc_rlimit(int nofile)
{
    struct rlimit rl;

    rl.rlim_cur = nofile;
    rl.rlim_max = nofile;

    return setrlimit(RLIMIT_NOFILE, &rl);
}

void Server::daemonize(int pidfile)
{
    if (0 != fork()) {
        exit(0);
    }
    if (-1 == setsid()) {
        exit(0);
    }
    signal(SIGHUP, SIG_IGN);

    if (0 != fork()) {
        exit(0);
    }
    if (pidfile > 0) {
        char pidstring[32];
        int len;
        len = sprintf(pidstring, "%i", (int)getpid());
        write(pidfile, pidstring, len);
        close(pidfile);
    }
}

void Server::wait()
{
    int pid;
    int state;

    if (m_HasREPL) {
        this->displayVersion();
    }

    while ((pid = waitpid(-1, &state, 0))) {
        if (errno == ECHILD) {
            break;
        } else {

            if (WIFEXITED(state)) {
                continue;
            }

            if (WIFSIGNALED(state)) {
                int idx_crash = m_PidIdxMapper[pid];
                fprintf(stderr, "[Crash] Worker %d has crashed :'(\n", idx_crash);

                if (this->initWorker(&idx_crash) == 0) {
                    return;
                }
            }
        }
    }
}

void Server::displayVersion()
{
#include "ASCII.h"
    fprintf(stdout, nidium_ascii, NIDIUM_SERVER_VERSION,
        __DATE__, __TIME__, getpid(), m_NWorkers);
}

int Server::initWorker(int *idx)
{
    if (!(*idx)) {
        *idx = ++m_WorkerIdx;
    }

    pid_t pid = 0;
    /* Execute the worker for the child process and returns 0 */
#ifndef NIDIUM_NO_FORK
    if ((pid = fork()) == 0) {
#endif
        Worker worker(*idx, (m_HasREPL && *idx == 1));

        setproctitle("Nidium-Server:<%s> (worker %d)",
            m_InstanceName ? m_InstanceName : "noname", *idx);

        worker.run(m_Args.argc, m_Args.argv, m_JSStrictMode);

        return 0;
#ifndef NIDIUM_NO_FORK
    }

    pidList.push_back(pid);
#endif

    m_PidIdxMapper[pid] = *idx;

    /* Parent process returns the pid */
    return pid;
}

int Server::Start(int argc, char *argv[])
{
#ifdef INIT_SETPROCTITLE_REPLACEMENT
    spt_init(argc, argv);
#endif

    Server *server = new Server(argc, argv);

    return server->init();
}

int Server::init()
{
    bool daemon = false;
    int workers = 1;

    static struct option long_options[] =
    {
        {"strict",     no_argument,       0, 's'},
        {"daemon",     no_argument,       0, 'd'},
        {"workers",    required_argument, 0, 'w'},
        {"name",       required_argument, 0, 'n'},
        {0, 0, 0, 0}
    };

    _ape_seed = time(NULL) ^ (getpid() << 16);

    signal(SIGINT, &signal_handler);
    signal(SIGTERM, &signal_handler);
    signal(SIGQUIT, &signal_handler);
    //signal(SIGCHLD, SIG_IGN);

    int ch;
    //opterr = 0;

    /*
        Needed on macosx so that arguments doesn't fail after the .js file
    */
    setenv("POSIXLY_CORRECT", "1", 1);

    while ((ch = getopt_long(m_Args.argc, m_Args.argv, "sdnw:", long_options, NULL)) != -1) {
        //printf("Got %c (%s)\n", ch, optarg);
        switch (ch) {
            case 'd':
                daemon = true;
                break;
            case 's':
                m_JSStrictMode = true;
                break;
            case '?':
                exit(1);
                break;
            case 'w':
                workers = atoi(optarg);
                break;
            case 'n':
                m_InstanceName = strdup(optarg);
                break;
            default:
                break;
        }
    }
    /*
        Only 'forward' the 'rest' of the arguments to the js Process/worker
    */
    m_Args.argc -= optind;
    m_Args.argv += optind;

    if (workers > NIDIUM_MAX_WORKERS) {
        fprintf(stderr, "[Error] Too many worker requested : max %d\n", NIDIUM_MAX_WORKERS);
        exit(1);
    }

    /*
        Don't demonize if no JS file was provided
    */
    if (daemon && m_Args.argc >= 1) {
        m_HasREPL = false;
        this->daemonize();
    } else if (daemon) {
        fprintf(stderr, "Can't demonize if no JS file is provided\n");
    }

    if (workers) {
        m_NWorkers = workers;
        for (int i = 0; i < workers; i++) {
            int idx = 0;
            if (this->initWorker(&idx) == 0) {
                return 1;
            }
        }
    }

    /*
        Only executed by the parent process
    */
    this->wait();

    return 1;
}

Server::Server(int argc, char **argv) :
    m_WorkerIdx(0), m_InstanceName(NULL), m_HasREPL(true),
    m_JSStrictMode(false), m_NWorkers(0)
{
    m_Args.argc = argc;
    m_Args.argv = argv;
}

Worker::Worker(int idx, bool repl) :
    m_Idx(idx), m_RunREPL(repl)
{

}

Worker::~Worker()
{

}

static int NidiumCheckParentAlive_ping(void *arg)
{
    pid_t ppid = getppid();
    /*
        If the parent's pid is 0 or 1, it means that our parent is dead. Exit.
    */
    if (ppid == 0 || ppid == 1) {
        exit(0);
    }

    return 1000;
}

int Worker::run(int argc, char **argv, bool jsstrict)
{
    REPL *repl = NULL;
    ape_global *net = APE_init();

    inc_rlimit(64000);

    signal(SIGPIPE, SIG_IGN);

    Context ctx(net, this, jsstrict, m_RunREPL);
    const Nidium::Binding::NidiumJS *js = ctx.getNJS();
    Nidium::Binding::JSProcess::registerObject(js->getJSContext(), argv, argc,
        this->getIdentifier());

    /*
        Daemon requires a .js to load
    */

#ifdef DEBUG
    fprintf(stdout, "[Warn] Running in Debug mode\n");
#endif
    if (jsstrict) {
        fprintf(stdout, "[JS] Strict mode is enabled\n");
    }
    if (argc >= 1) {
        Nidium::Binding::NidiumJS *js = ctx.getNJS();
        if (!js) {
            fprintf(stderr, "Failed to get JS\n");
            return 0;
        }
        ctx.getNJS()->LoadScript(argv[0]);
    }

    /* Heap allocated because we need to be
    sure that it's deleted before Nidium::Binding::NidiumJS */
    if (m_RunREPL) {
        repl = new REPL(ctx.getNJS());
    }

    APE_timer_create(net, 1, NidiumCheckParentAlive_ping, NULL);
    APE_loop_run(net);

    if (repl) {
        delete repl;
    }
#if 0
    TODO : heap use after free ?
    APE_destroy(net);
#endif
    return 0;
}

} // namespace Server
} // namespace Nidium
