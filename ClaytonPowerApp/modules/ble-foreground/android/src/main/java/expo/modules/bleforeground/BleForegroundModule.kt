package expo.modules.bleforeground

import android.content.Context
import android.content.Intent
import android.os.Build
import expo.modules.kotlin.modules.Module
import expo.modules.kotlin.modules.ModuleDefinition

class BleForegroundModule : Module() {
  private val context: Context
    get() = requireNotNull(appContext.reactContext) { "React context is not available" }

  override fun definition() = ModuleDefinition {
    Name("BleForeground")

    // Start the service, or update its notification text when already running.
    // Must first be called while the app is in the foreground (Android 12+).
    Function("start") { title: String, text: String ->
      val intent = Intent(context, BleForegroundService::class.java)
        .putExtra(BleForegroundService.EXTRA_TITLE, title)
        .putExtra(BleForegroundService.EXTRA_TEXT, text)
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        context.startForegroundService(intent)
      } else {
        context.startService(intent)
      }
    }

    // Only changes the text of the running notification; safe from the background.
    Function("update") { title: String, text: String ->
      BleForegroundService.updateNotification(context, title, text)
    }

    Function("isRunning") {
      BleForegroundService.running
    }

    Function("stop") {
      context.stopService(Intent(context, BleForegroundService::class.java))
    }
  }
}
