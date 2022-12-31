import 'package:firebase_auth/firebase_auth.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:rxdart/rxdart.dart';

class FirebaseManager {
  final auth = FirebaseAuth.instance;
  final databaseRef = FirebaseDatabase.instance;
  final BehaviorSubject<String> _boilerMsg = BehaviorSubject();

  final BehaviorSubject<User> _loginAuth = BehaviorSubject();

  Stream<User> validate({required String email, required String password}) {
    // try {
    //   // getBoilerMsg(); //Todo: remove

    //   final userCredential = await
    //   if (userCredential.user == null) {
    //     throw Exception("User login failed. Please try again");
    //   }
    //   return ApiResult(data: userCredential.user);
    // } on Exception catch (e) {
    //   return ApiResult(error: e);
    // }

    auth
        .signInWithEmailAndPassword(email: email, password: password)
        .then((value) {
      _loginAuth.sink.add(value.user!);
    }).onError((error, stackTrace) {
      _loginAuth.addError(stackTrace);
    });
    return _loginAuth.stream;
  }

  Stream<String> getBoilerMsg() {
    List<String> values = [];
    final boilerMsg = databaseRef.ref('BoilerOpsRT').child('boilerMsg');
    boilerMsg.onChildChanged.listen((event) {
      final snapshot = event.snapshot.value.toString();
      //values.add(snapshot);
      if (event.snapshot.key == 'sample') {
        _boilerMsg.sink.add(snapshot);
      }

      //print("newchangeobject: $snapshot");
      // final result = BoilerMsg()
      // _boilerMsg.sink.add();
    });
    return _boilerMsg.stream;
  }

  void getData() {
    final data = databaseRef.ref('data');
  }
}

/*
{
  "boilerMsg": {
    "sample": 32076,
    "sampleDT": 1672417370663
  },
  "data": {
    "boiler": 0,
    "z1": 0,
    "z2": 0,
    "z3": 1,
    "z4": 0,
    "z5": 0,
    "z6": 1
  }
}

*/

class ApiResult<T> {
  T? data;
  Exception? error;

  ApiResult({this.data, this.error});
}
