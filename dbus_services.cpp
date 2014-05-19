#include "dbus_service.h"
#include "dbus_services.h"
#include "defines.h"

//#define QS_LOG_DISABLE
#include "QsLog.h"

DBusServices::DBusServices(QObject *parent) :
	QObject(parent),
	mDBus(DBUS_CONNECTION)
{
}

void DBusServices::initialScan()
{
	QDBusConnectionInterface *interface = mDBus.interface();
	QStringList serviceNames = interface->registeredServiceNames();
	connect(interface, SIGNAL(serviceOwnerChanged(QString,QString,QString)), SLOT(serviceOwnerChanged(QString,QString,QString)));

	foreach (const QString &name, serviceNames)
		processServiceName(name);
}

DBusService * DBusServices::getService(DBusService::DbusServiceType deviceType, int deviceInstance)
{
	if (mServiceByType.contains(deviceType)) {
		QMap<int, DBusService *> device = mServiceByType.value(deviceType);
		if (device.contains(deviceInstance))
			return device.value(deviceInstance);
	}
	return 0;
}

void DBusServices::processServiceName(QString name)
{
	if (!name.startsWith("com.victronenergy."))
		return;

	if (mServicesByName.contains(name)) {
		QLOG_TRACE() << "[DBusServices] connect " << name;
		mServicesByName.value(name)->setConnected(true);
		return;
	}

	QLOG_TRACE() << "[DBusServices] Add new service " << name;
	DBusService *service = new DBusService(name);
	if (service == 0)
		return;
	mServicesByName.insert(name, service);
	QMap<int, DBusService *> instance;
	instance.insert(service->getDeviceInstance(), service);
	mServiceByType.insert(service->getDeviceType(name), instance);
	emit dbusServiceFound(service);
}

void DBusServices::serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
{
	Q_UNUSED(oldOwner);
	if (!newOwner.isEmpty() ) {
		// new owner > service add on dbus
		processServiceName(name);
	} else {
		// no new owner > service removed from dbus
		if (mServicesByName.contains(name)) {
			QLOG_TRACE() << "[DBusServices] disconnect " << name;
			mServicesByName.value(name)->setConnected(false);
		}
	}
}