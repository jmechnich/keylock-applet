#ifndef SIGNAL_HH
#define SIGNAL_HH

#include <QSocketNotifier>

#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>

#include <map>

class Signal : public QObject
{
  Q_OBJECT
public:
  Signal(int signum, QObject* parent)
          : QObject(parent)
          , _signum(signum)
          , _sn(0)
        {
          _fd[0] = -1;
          _fd[1] = -1;

          if( setupHandler() < 0)
              return;

          _sn = new QSocketNotifier(_fd[1], QSocketNotifier::Read, parent);
          connect(_sn, SIGNAL(activated(int)), this, SLOT(handleSignal()));
        }

  ~Signal()
        {
          ::signal(_signum,SIG_DFL);
          std::map<int,Signal*>::iterator it = _fds.find(_signum);
          if( it != _fds.end())
              _fds.erase(it);
          if( _fd[0] > 0)
              close(_fd[0]);
          if( _fd[1] > 0)
              close(_fd[1]);
        }
  
  static Signal* create(int signum, QObject* parent)
        {
          std::map<int,Signal*>::iterator it = _fds.find(signum);
          if( it != _fds.end())
          {
            if( it->second->_sn)
                delete it->second->_sn;
            delete it->second;
          }
          return new Signal(signum,parent);
        }
  
signals:
  void signal(int);
                  
private slots:
  void handleSignal()
        {
          _sn->setEnabled(false);
          char tmp;
          ::read(_fd[1], &tmp, sizeof(tmp));
          emit signal(_signum);
          _sn->setEnabled(true);
        }
private:
  int setupHandler()
        {
          if (::socketpair(AF_UNIX, SOCK_STREAM, 0, _fd))
              return -1;
          _fds[_signum] = this;
          struct sigaction a;
          a.sa_handler = &Signal::handler;
          sigemptyset(&a.sa_mask);
          a.sa_flags = 0;
          a.sa_flags |= SA_RESTART;
          if( sigaction(_signum, &a, 0) > 0) return -1;
          return 0;
        }

  static void handler(int signum)
        {
          char a = 1;
          ::write(_fds[signum]->_fd[0], &a, sizeof(a));
        }

  int _signum;
  QSocketNotifier* _sn;
  int _fd[2];
  static std::map<int,Signal*> _fds;
};

#endif
