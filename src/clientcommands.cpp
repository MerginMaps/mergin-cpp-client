/*
 * mergin-cpp-client
 * Copyright (C) 2021 Lutra Consulting
 * License: MIT
 */

#include "clientcommands.h"
#include <QEventLoop>
#include <QTimer>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>

#include "merginuserauth.h"

ClientCommands::ClientCommands( const QString &dir, int timeout ):
  mTimeout( timeout ),
  mLocalProjectsManager( dir ),
  mApi( mLocalProjectsManager )
{
}

void ClientCommands::login( const QString &url, const QString &user, const QString &pass )
{
  if ( url.isEmpty() )
    mApi.setApiRoot( MerginApi::defaultApiRoot() );
  else
    mApi.setApiRoot( url );

  QTimer timer;
  timer.setSingleShot( true );
  QEventLoop loop;

  connect( &mApi, &MerginApi::authFailed, &loop, &QEventLoop::quit );
  connect( &mApi, &MerginApi::authChanged, &loop, &QEventLoop::quit );
  connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
  timer.start( mTimeout );
  mApi.authorize( user, pass );
  loop.exec();

  if ( !timer.isActive() )
    throw QString( "timeout for login" );

  if ( !isAuthorized() )
    throw QString( "not authorized" );
}

void ClientCommands::create( const QString &projectNamespace, const QString &projectName )
{
  Q_ASSERT( isAuthorized() );
  QTimer timer;
  timer.setSingleShot( true );
  QEventLoop loop;

  connect( &mApi, &MerginApi::projectCreated, &loop, &QEventLoop::quit );
  connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
  timer.start( mTimeout );
  mApi.createProject( projectNamespace, projectName );
  loop.exec();

  if ( !timer.isActive() )
    throw QString( "timeout for create" );
}

void ClientCommands::remove( const QString &projectNamespace, const QString &projectName )
{
  Q_ASSERT( isAuthorized() );
  QTimer timer;
  timer.setSingleShot( true );
  QEventLoop loop;

  connect( &mApi, &MerginApi::serverProjectDeleted, &loop, &QEventLoop::quit );
  connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
  timer.start( mTimeout );
  mApi.deleteProject( projectNamespace, projectName );
  loop.exec();

  if ( !timer.isActive() )
    throw QString( "timeout for delete" );

}

void ClientCommands::pull( const QString &projectNamespace, const QString &projectName )
{
  Q_ASSERT( isAuthorized() );

  QTimer timer;
  timer.setSingleShot( true );
  QEventLoop loop;

  connect( &mApi, &MerginApi::syncProjectFinished, &loop, &QEventLoop::quit );
  connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
  timer.start( mTimeout );
  mApi.pullProject( projectNamespace, projectName );
  loop.exec();

  if ( !timer.isActive() )
    throw QString( "timeout for pull" );
}

void ClientCommands::push( const QString &projectNamespace, const QString &projectName )
{
  Q_ASSERT( isAuthorized() );

  QTimer timer;
  timer.setSingleShot( true );
  QEventLoop loop;

  connect( &mApi, &MerginApi::syncProjectFinished, &loop, &QEventLoop::quit );
  connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
  timer.start( mTimeout );
  mApi.pushProject( projectNamespace, projectName );
  loop.exec();

  if ( !timer.isActive() )
    throw QString( "timeout for push" );
}

void ClientCommands::sync()
{
  Q_ASSERT( isAuthorized() );
  LocalProject lp = mLocalProjectsManager.projectFromDirectory( QDir::currentPath() );
  if ( !lp.isValid() )
    throw QString( "no mergin project in the current directory" );
  push( lp.projectNamespace, lp.projectName );
}

void ClientCommands::info( bool isJsonFormat )
{
  LocalProject lp = mLocalProjectsManager.projectFromDirectory( QDir::currentPath() );
  if ( !lp.isValid() )
    throw QString( "no mergin project in the current directory" );

  QTextStream out( stdout );
  if ( isJsonFormat )
  {
    QJsonObject jsonData
    {
      {"name", QJsonValue( lp.projectName )},
      {"namespace", QJsonValue( lp.projectNamespace )},
      {"id", QJsonValue( lp.id() )},
      {"localVersion", QJsonValue( lp.localVersion )}
    };

    QJsonDocument doc( jsonData );
    out << doc.toJson( QJsonDocument::Compact );
  }
  else
  {
    out << "name: " << lp.projectName << endl;
    out << "namespace: " << lp.projectNamespace << endl;
    out << "id: " << lp.id() << endl;
    out << "localVersion: " << lp.localVersion << endl;
  }
}


bool ClientCommands::isAuthorized()
{
  return !mApi.userAuth()->authToken().isEmpty();
}
