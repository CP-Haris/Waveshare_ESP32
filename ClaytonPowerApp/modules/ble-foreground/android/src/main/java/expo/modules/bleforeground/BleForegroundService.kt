package expo.modules.bleforeground

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder

/**
 * Keeps the app process — and with it the JS BLE link — alive while the app is
 * in the background. All Bluetooth work stays in JS; this service only owns the
 * ongoing notification Android requires for a connectedDevice foreground service.
 */
class BleForegroundService : Service() {
  override fun onBind(intent: Intent?): IBinder? = null

  override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
    val title = intent?.getStringExtra(EXTRA_TITLE) ?: "Clayton Power"
    val text = intent?.getStringExtra(EXTRA_TEXT) ?: ""
    val notification = buildNotification(this, title, text)

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
      startForeground(NOTIFICATION_ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE)
    } else {
      startForeground(NOTIFICATION_ID, notification)
    }
    running = true
    // Restarted by Android if killed under memory pressure; JS re-attaches on launch.
    return START_STICKY
  }

  override fun onDestroy() {
    running = false
    super.onDestroy()
  }

  companion object {
    const val EXTRA_TITLE = "title"
    const val EXTRA_TEXT = "text"
    private const val CHANNEL_ID = "ble-background"
    private const val NOTIFICATION_ID = 4210

    @Volatile
    var running = false
      private set

    /** Change the text of the running service's notification (allowed from the background). */
    fun updateNotification(context: Context, title: String, text: String) {
      if (!running) return
      context.getSystemService(NotificationManager::class.java)
        .notify(NOTIFICATION_ID, buildNotification(context, title, text))
    }

    private fun buildNotification(context: Context, title: String, text: String): Notification {
      val manager = context.getSystemService(NotificationManager::class.java)
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        val channel = NotificationChannel(CHANNEL_ID, "Background connection", NotificationManager.IMPORTANCE_LOW)
        channel.description = "Shown while the app stays connected in the background"
        channel.setShowBadge(false)
        manager.createNotificationChannel(channel)
      }

      val launch = context.packageManager.getLaunchIntentForPackage(context.packageName)
      val openApp = PendingIntent.getActivity(
        context, 0, launch,
        PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
      )

      val builder = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        Notification.Builder(context, CHANNEL_ID)
      } else {
        @Suppress("DEPRECATION")
        Notification.Builder(context)
      }

      return builder
        .setSmallIcon(smallIcon(context))
        .setContentTitle(title)
        .setContentText(text)
        .setContentIntent(openApp)
        .setOngoing(true)
        .setCategory(Notification.CATEGORY_SERVICE)
        .build()
    }

    // The monochrome icon expo-notifications installs; app icon as a fallback.
    private fun smallIcon(context: Context): Int {
      val id = context.resources.getIdentifier("notification_icon", "drawable", context.packageName)
      return if (id != 0) id else context.applicationInfo.icon
    }
  }
}
