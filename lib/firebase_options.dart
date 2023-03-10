// File generated by FlutterFire CLI.
// ignore_for_file: lines_longer_than_80_chars, avoid_classes_with_only_static_members
import 'package:firebase_core/firebase_core.dart' show FirebaseOptions;
import 'package:flutter/foundation.dart'
    show defaultTargetPlatform, kIsWeb, TargetPlatform;

/// Default [FirebaseOptions] for use with your Firebase apps.
///
/// Example:
/// ```dart
/// import 'firebase_options.dart';
/// // ...
/// await Firebase.initializeApp(
///   options: DefaultFirebaseOptions.currentPlatform,
/// );
/// ```
class DefaultFirebaseOptions {
  static FirebaseOptions get currentPlatform {
    if (kIsWeb) {
      return web;
    }
    switch (defaultTargetPlatform) {
      case TargetPlatform.android:
        return android;
      case TargetPlatform.iOS:
        return ios;
      case TargetPlatform.macOS:
        return macos;
      case TargetPlatform.windows:
        throw UnsupportedError(
          'DefaultFirebaseOptions have not been configured for windows - '
          'you can reconfigure this by running the FlutterFire CLI again.',
        );
      case TargetPlatform.linux:
        throw UnsupportedError(
          'DefaultFirebaseOptions have not been configured for linux - '
          'you can reconfigure this by running the FlutterFire CLI again.',
        );
      default:
        throw UnsupportedError(
          'DefaultFirebaseOptions are not supported for this platform.',
        );
    }
  }

  static const FirebaseOptions web = FirebaseOptions(
    apiKey: 'AIzaSyCLMu0cAV7PjL9oJkNyUTw9fb-cDGIkkXk',
    appId: '1:969606937132:web:e63ab6bf516b0fdffaaf89',
    messagingSenderId: '969606937132',
    projectId: 'apponaugdata',
    authDomain: 'apponaugdata.firebaseapp.com',
    databaseURL: 'https://apponaugdata-default-rtdb.firebaseio.com',
    storageBucket: 'apponaugdata.appspot.com',
  );

  static const FirebaseOptions android = FirebaseOptions(
    apiKey: 'AIzaSyD0Rbh-8cB1PSgm4zX5lduhujB_MnVDU6k',
    appId: '1:969606937132:android:33df172bc6e26b61faaf89',
    messagingSenderId: '969606937132',
    projectId: 'apponaugdata',
    databaseURL: 'https://apponaugdata-default-rtdb.firebaseio.com',
    storageBucket: 'apponaugdata.appspot.com',
  );

  static const FirebaseOptions ios = FirebaseOptions(
    apiKey: 'AIzaSyAyl7U3NqzyxOtjUTBzP9MiKoUhpGwZh8s',
    appId: '1:969606937132:ios:18663019a1e0387efaaf89',
    messagingSenderId: '969606937132',
    projectId: 'apponaugdata',
    databaseURL: 'https://apponaugdata-default-rtdb.firebaseio.com',
    storageBucket: 'apponaugdata.appspot.com',
    iosClientId: '969606937132-tc6g36hhic6r7lt1r1d81jdtqhsfcghn.apps.googleusercontent.com',
    iosBundleId: 'com.example.boilerstat',
  );

  static const FirebaseOptions macos = FirebaseOptions(
    apiKey: 'AIzaSyAyl7U3NqzyxOtjUTBzP9MiKoUhpGwZh8s',
    appId: '1:969606937132:ios:18663019a1e0387efaaf89',
    messagingSenderId: '969606937132',
    projectId: 'apponaugdata',
    databaseURL: 'https://apponaugdata-default-rtdb.firebaseio.com',
    storageBucket: 'apponaugdata.appspot.com',
    iosClientId: '969606937132-tc6g36hhic6r7lt1r1d81jdtqhsfcghn.apps.googleusercontent.com',
    iosBundleId: 'com.example.boilerstat',
  );
}
